#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <zlib.h>

#include <Eina.h>
#include "Emile.h"

#ifdef ENABLE_LIBLZ4
# include <lz4.h>
# include <lz4hc.h>
#else
# include "lz4.h"
# include "lz4hc.h"
#endif

static int
emile_compress_buffer_size(const Eina_Binbuf *data, Emile_Compressor_Type t)
{
   switch (t)
     {
      case EMILE_ZLIB:
         return 12 + ((eina_binbuf_length_get(data) * 101) / 100);
      case EMILE_LZ4:
      case EMILE_LZ4HC:
         return LZ4_compressBound(eina_binbuf_length_get(data));
      default:
         return -1;
     }
}

EAPI Eina_Binbuf *
emile_binbuf_compress(const Eina_Binbuf *data, Emile_Compressor_Type t, int level)
{
   void *compact;
   int length;
   Eina_Bool ok = EINA_FALSE;

   length = emile_compress_buffer_size(data, t);

   compact = malloc(length);
   if (!compact) return NULL;

   switch (t)
     {
      case EMILE_LZ4:
         length = LZ4_compress((const char *) eina_binbuf_string_get(data), compact, eina_binbuf_length_get(data));
         if (length > 0) ok = EINA_TRUE;
         compact = realloc(compact, length); // It is going to be smaller and should never fail, if it does you are in deep poo.
         break;
      case EMILE_LZ4HC:
         length = LZ4_compressHC((const char *) eina_binbuf_string_get(data), compact, eina_binbuf_length_get(data));
         if (length > 0) ok = EINA_TRUE;
         compact = realloc(compact, length);
         break;
      case EMILE_ZLIB:
        {
           uLongf buflen = (uLongf) length;

           if (compress2((Bytef *)compact, &buflen, (Bytef *) eina_binbuf_string_get(data),
                         (uLong) eina_binbuf_length_get(data), level) == Z_OK)
             ok = EINA_TRUE;
           length = (int) buflen;
        }
     }

   if (!ok)
     {
        free(compact);
        return NULL;
     }

   return eina_binbuf_manage_new_length(compact, length);
}

EAPI Eina_Bool
emile_binbuf_expand(const Eina_Binbuf *in,
                    Eina_Binbuf *out,
                    Emile_Compressor_Type t)
{
   if (!in || !out) return EINA_FALSE;

   switch (t)
     {
      case EMILE_LZ4:
      case EMILE_LZ4HC:
        {
           int ret;

           ret = LZ4_uncompress((const char*) eina_binbuf_string_get(in),
                                (char*) eina_binbuf_string_get(out),
                                eina_binbuf_length_get(out));
           if ((unsigned int) ret != eina_binbuf_length_get(in))
             return EINA_FALSE;
           break;
        }
      case EMILE_ZLIB:
        {
           uLongf dlen = eina_binbuf_length_get(out);

           if (uncompress((Bytef *) eina_binbuf_string_get(out), &dlen,
                          eina_binbuf_string_get(in),
                          (uLongf) eina_binbuf_length_get(in)) != Z_OK)
             return EINA_FALSE;
           break;
        }
      default:
         return EINA_FALSE;
     }

   return EINA_TRUE;
}

EAPI Eina_Binbuf *
emile_binbuf_uncompress(const Eina_Binbuf *data, Emile_Compressor_Type t, unsigned int dest_length)
{
   Eina_Binbuf *out;
   void *expanded;

   expanded = malloc(dest_length);
   if (!expanded) return NULL;

   out = eina_binbuf_manage_new_length(expanded, dest_length);
   if (!out) goto on_error;

   if (!emile_binbuf_expand(data, out, t))
     goto on_error;

   return out;

 on_error:
   if (!out) free(expanded);
   if (out) eina_binbuf_free(out);
   return NULL;
}
