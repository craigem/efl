#include "evas_common_private.h"

#ifdef USE_HARFBUZZ
# include <hb.h>
# include <hb-ft.h>
#endif

#include "evas_font_private.h"

#ifdef USE_HARFBUZZ
static const hb_script_t
_evas_script_to_harfbuzz[] =
{
  HB_SCRIPT_COMMON,
  HB_SCRIPT_INHERITED,
  HB_SCRIPT_ARABIC,
  HB_SCRIPT_ARMENIAN,
  HB_SCRIPT_BENGALI,
  HB_SCRIPT_BOPOMOFO,
  HB_SCRIPT_CHEROKEE,
  HB_SCRIPT_COPTIC,
  HB_SCRIPT_CYRILLIC,
  HB_SCRIPT_DESERET,
  HB_SCRIPT_DEVANAGARI,
  HB_SCRIPT_ETHIOPIC,
  HB_SCRIPT_GEORGIAN,
  HB_SCRIPT_GOTHIC,
  HB_SCRIPT_GREEK,
  HB_SCRIPT_GUJARATI,
  HB_SCRIPT_GURMUKHI,
  HB_SCRIPT_HAN,
  HB_SCRIPT_HANGUL,
  HB_SCRIPT_HEBREW,
  HB_SCRIPT_HIRAGANA,
  HB_SCRIPT_KANNADA,
  HB_SCRIPT_KATAKANA,
  HB_SCRIPT_KHMER,
  HB_SCRIPT_LAO,
  HB_SCRIPT_LATIN,
  HB_SCRIPT_MALAYALAM,
  HB_SCRIPT_MONGOLIAN,
  HB_SCRIPT_MYANMAR,
  HB_SCRIPT_OGHAM,
  HB_SCRIPT_OLD_ITALIC,
  HB_SCRIPT_ORIYA,
  HB_SCRIPT_RUNIC,
  HB_SCRIPT_SINHALA,
  HB_SCRIPT_SYRIAC,
  HB_SCRIPT_TAMIL,
  HB_SCRIPT_TELUGU,
  HB_SCRIPT_THAANA,
  HB_SCRIPT_THAI,
  HB_SCRIPT_TIBETAN,
  HB_SCRIPT_CANADIAN_ABORIGINAL,
  HB_SCRIPT_YI,
  HB_SCRIPT_TAGALOG,
  HB_SCRIPT_HANUNOO,
  HB_SCRIPT_BUHID,
  HB_SCRIPT_TAGBANWA,

  /* Unicode-4.0 additions */
  HB_SCRIPT_BRAILLE,
  HB_SCRIPT_CYPRIOT,
  HB_SCRIPT_LIMBU,
  HB_SCRIPT_OSMANYA,
  HB_SCRIPT_SHAVIAN,
  HB_SCRIPT_LINEAR_B,
  HB_SCRIPT_TAI_LE,
  HB_SCRIPT_UGARITIC,

  /* Unicode-4.1 additions */
  HB_SCRIPT_NEW_TAI_LUE,
  HB_SCRIPT_BUGINESE,
  HB_SCRIPT_GLAGOLITIC,
  HB_SCRIPT_TIFINAGH,
  HB_SCRIPT_SYLOTI_NAGRI,
  HB_SCRIPT_OLD_PERSIAN,
  HB_SCRIPT_KHAROSHTHI,

  /* Unicode-5.0 additions */
  HB_SCRIPT_UNKNOWN,
  HB_SCRIPT_BALINESE,
  HB_SCRIPT_CUNEIFORM,
  HB_SCRIPT_PHOENICIAN,
  HB_SCRIPT_PHAGS_PA,
  HB_SCRIPT_NKO,

  /* Unicode-5.1 additions */
  HB_SCRIPT_KAYAH_LI,
  HB_SCRIPT_LEPCHA,
  HB_SCRIPT_REJANG,
  HB_SCRIPT_SUNDANESE,
  HB_SCRIPT_SAURASHTRA,
  HB_SCRIPT_CHAM,
  HB_SCRIPT_OL_CHIKI,
  HB_SCRIPT_VAI,
  HB_SCRIPT_CARIAN,
  HB_SCRIPT_LYCIAN,
  HB_SCRIPT_LYDIAN,

  /* Unicode-5.2 additions */
  HB_SCRIPT_AVESTAN,
  HB_SCRIPT_BAMUM,
  HB_SCRIPT_EGYPTIAN_HIEROGLYPHS,
  HB_SCRIPT_IMPERIAL_ARAMAIC,
  HB_SCRIPT_INSCRIPTIONAL_PAHLAVI,
  HB_SCRIPT_INSCRIPTIONAL_PARTHIAN,
  HB_SCRIPT_JAVANESE,
  HB_SCRIPT_KAITHI,
  HB_SCRIPT_TAI_THAM,
  HB_SCRIPT_LISU,
  HB_SCRIPT_MEETEI_MAYEK,
  HB_SCRIPT_OLD_SOUTH_ARABIAN,
  HB_SCRIPT_OLD_TURKIC,
  HB_SCRIPT_SAMARITAN,
  HB_SCRIPT_TAI_VIET,

  /* Unicode-6.0 additions */
  HB_SCRIPT_BATAK,
  HB_SCRIPT_BRAHMI,
  HB_SCRIPT_MANDAIC
};
#endif

#ifdef OT_SUPPORT
/* FIXME: doc. returns #items */
EAPI int
evas_common_font_ot_cluster_size_get(const Evas_Text_Props *props, size_t char_index)
{
   int i;
   int items;
   int left_bound, right_bound;
   size_t base_cluster;
   char_index += props->start;
   base_cluster = EVAS_FONT_OT_POS_GET(props->info->ot[char_index]);
   for (i = (int) char_index ;
         (i >= (int) props->start) &&
         (EVAS_FONT_OT_POS_GET(props->info->ot[i]) == base_cluster) ;
         i--)
     ;
   left_bound = i;
   for (i = (int) char_index + 1;
         (i < (int) (props->start + props->len)) &&
         (EVAS_FONT_OT_POS_GET(props->info->ot[i]) == base_cluster) ;
         i++)
     ;
   right_bound = i;

   if (right_bound == left_bound)
     {
        items = 1;
     }
   else if (props->bidi_dir == EVAS_BIDI_DIRECTION_RTL)
     {
        if (left_bound < 0)
          {
             items = props->text_offset + props->text_len - base_cluster;
          }
        else
          {
             items = props->info->ot[left_bound].source_cluster - base_cluster;
          }
     }
   else
     {
        if (right_bound >= (int) (props->start + props->len))
          {
             items = props->text_offset + props->text_len - base_cluster;
          }
        else
          {
             items = props->info->ot[right_bound].source_cluster - base_cluster;
          }
     }
   return (items > 0) ? items : 1;
}

/* Harfbuzz font functions */

static hb_position_t
_evas_common_font_ot_hb_get_glyph_advance(hb_font_t *font,
      void *font_data, hb_codepoint_t glyph,
      void *user_data)
{
   /* Use our cache*/
   RGBA_Font_Int *fi = (RGBA_Font_Int *) font_data;
   RGBA_Font_Glyph *fg;
   (void) font;
   (void) user_data;
   fg = evas_common_font_int_cache_glyph_get(fi, glyph);
   if (fg)
     {
        return fg->glyph->advance.x >> 10;
     }
   return 0;
}

static hb_position_t
_evas_common_font_ot_hb_get_kerning(hb_font_t *font, void *font_data,
   hb_codepoint_t first_glyph, hb_codepoint_t second_glyph, void *user_data)
{
   RGBA_Font_Int *fi = (RGBA_Font_Int *) font_data;
   int kern;
   (void) font;
   (void) user_data;
   if (evas_common_font_query_kerning(fi, first_glyph, second_glyph, &kern))
      return kern;

   return 0;
}

/* End of harfbuzz font funcs */

static inline hb_font_funcs_t *
_evas_common_font_ot_font_funcs_get(void)
{
   static hb_font_funcs_t *font_funcs = NULL;
   if (!font_funcs)
     {
        font_funcs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_h_advance_func(font_funcs,
            _evas_common_font_ot_hb_get_glyph_advance, NULL, NULL);
        hb_font_funcs_set_glyph_h_kerning_func(font_funcs,
            _evas_common_font_ot_hb_get_kerning, NULL, NULL);
     }

   return font_funcs;
}

static inline hb_unicode_funcs_t *
_evas_common_font_ot_unicode_funcs_get(void)
{
   static hb_unicode_funcs_t *unicode_funcs = NULL;
   if (!unicode_funcs)
     {
        unicode_funcs = hb_unicode_funcs_get_default();
     }

   return unicode_funcs;
}

static void
_evas_common_font_ot_shape(hb_buffer_t *buffer, RGBA_Font_Int *fi, Evas_Text_Props_Mode mode)
{
   /* Create hb_font if not previously created */
   if (!fi->ft.hb_font)
     {
        hb_font_t *hb_ft_font;

        hb_ft_font = hb_ft_font_create(fi->src->ft.face, NULL);
        fi->ft.hb_font = hb_font_create_sub_font(hb_ft_font);
        hb_font_destroy(hb_ft_font);

        hb_font_set_funcs(fi->ft.hb_font,
              _evas_common_font_ot_font_funcs_get(), fi, NULL);
     }

   if (mode == EVAS_TEXT_PROPS_MODE_SHAPE)
     {
        hb_shape(fi->ft.hb_font, buffer, NULL, 0);
     }
   else
     {
        const char *shaper_list[] = { "fallback", NULL };
        hb_shape_full(fi->ft.hb_font, buffer, NULL, 0, shaper_list);
     }
}

EAPI Eina_Bool
evas_common_font_ot_populate_text_props(const Eina_Unicode *text,
      Evas_Text_Props *props, int len, Evas_Text_Props_Mode mode)
{
   RGBA_Font_Int *fi;
   hb_buffer_t *buffer;
   hb_glyph_position_t *positions;
   hb_glyph_info_t *infos;
   int slen;
   unsigned int i;
   Evas_Font_Glyph_Info *gl_itr;
   Evas_Font_OT_Info *ot_itr;
   Evas_Coord pen_x = 0;

   fi = props->font_instance;

   if (len < 0)
     {
        slen = eina_unicode_strlen(text);
     }
   else
     {
        slen = len;
     }

   buffer = hb_buffer_create();
   hb_buffer_set_unicode_funcs(buffer, _evas_common_font_ot_unicode_funcs_get());
   hb_buffer_set_language(buffer, hb_language_from_string(
            evas_common_language_from_locale_get(), -1));
   hb_buffer_set_script(buffer, _evas_script_to_harfbuzz[props->script]);
   hb_buffer_set_direction(buffer,
         (props->bidi_dir == EVAS_BIDI_DIRECTION_RTL) ?
         HB_DIRECTION_RTL : HB_DIRECTION_LTR);
   /* FIXME: add run-time conversions if needed, which is very unlikely */
   hb_buffer_add_utf32(buffer, (const uint32_t *) text, slen, 0, slen);

   _evas_common_font_ot_shape(buffer, fi, mode);

   /* info_len stores glyph information array's length
    * (this is for handling split items) */
   props->info->len = props->len = hb_buffer_get_length(buffer);
   props->info->ot = calloc(props->len,
         sizeof(Evas_Font_OT_Info));
   props->info->glyph = calloc(props->len,
              sizeof(Evas_Font_Glyph_Info));
   positions = hb_buffer_get_glyph_positions(buffer, NULL);
   infos = hb_buffer_get_glyph_infos(buffer, NULL);
   gl_itr = props->info->glyph;
   ot_itr = props->info->ot;
   for (i = 0 ; i < props->len ; i++)
     {
        Evas_Coord adv;
        ot_itr->source_cluster = infos->cluster;
        ot_itr->x_offset = positions->x_offset;
        ot_itr->y_offset = positions->y_offset;
        gl_itr->index = infos->codepoint;
        adv = positions->x_advance;

        pen_x += adv;
        gl_itr->pen_after = EVAS_FONT_ROUND_26_6_TO_INT(pen_x);

        ot_itr++;
        gl_itr++;
        infos++;
        positions++;
     }

   hb_buffer_destroy(buffer);
   evas_common_font_int_use_trim();

   return EINA_FALSE;
}

/* Like ot_populate, but instead just updates a specific range in the
 * glyph information for a single item
 */
EAPI Eina_Bool
evas_common_font_ot_update_text_props(const Eina_Unicode *text,
      Evas_Text_Props *props, size_t len, int pos, Evas_Text_Props_Mode mode)
{
   RGBA_Font_Int *fi;
   hb_buffer_t *buffer;
   hb_glyph_position_t *positions;
   hb_glyph_info_t *infos;
   int text_len = eina_unicode_strlen(text);
   int glen;
   unsigned int i;
   Evas_Font_Glyph_Info *gl_itr;
   Evas_Font_OT_Info *ot_itr;
   int pen_after = 0;
   fi = props->font_instance;

   int adv;
   int prev_pen_after;
   int clust_diff;

   int len1, offt, offs, len2;

   buffer = hb_buffer_create();
   hb_buffer_set_unicode_funcs(buffer, _evas_common_font_ot_unicode_funcs_get());
   hb_buffer_set_language(buffer, hb_language_from_string(
            evas_common_language_from_locale_get(), -1));
   hb_buffer_set_script(buffer, _evas_script_to_harfbuzz[props->script]);
   hb_buffer_set_direction(buffer,
         (props->bidi_dir == EVAS_BIDI_DIRECTION_RTL) ?
         HB_DIRECTION_RTL : HB_DIRECTION_LTR);
   /* reshape the item part in the unicode string */
   hb_buffer_add_utf32(buffer, (const uint32_t *) text, text_len,
         props->text_offset, len); //offset and length of item in node's string
   _evas_common_font_ot_shape(buffer, fi, mode);

   glen = hb_buffer_get_length(buffer); //get glyph length of text

   /* text props has not been freed. allocate larger space and copy content to
    * relevant range. */
   offs = props->start + props->len; // offset in source info of remaining items after updated items (before update)
   props->info->len += (glen - props->len);
   props->len = glen;
   len1 = props->start; // up to item's position
   offt = props->start + props->len; //offset in target info of remaining items after changed item
   len2 = props->info->len - // length of info of all items (like split ones)
      (props->start + // offset position of item in info
       props->len); // length of item (after update)
   {
      Evas_Font_OT_Info *tmp_ot = props->info->ot;
      Evas_Font_Glyph_Info *tmp_glyph = props->info->glyph;

      /* Allocate space for updated info.
       * Size might be smaller (forming of ligatures),
       * or bigger than original length */
      props->info->ot = calloc(props->info->len,
            sizeof(Evas_Font_OT_Info));
      props->info->glyph = calloc(props->info->len,
            sizeof(Evas_Font_Glyph_Info));

      /* copy FIRST non-modified section */
      if (len1 > 0)
        {
           memcpy(props->info->ot, tmp_ot,
                 len1 * sizeof(Evas_Font_OT_Info));
           memcpy(props->info->glyph, tmp_glyph,
                 len1 * sizeof(Evas_Font_Glyph_Info));
        }

      /* copy LAST non-modified section */
      if (len2 > 0)
        {
           memcpy(props->info->ot + offt, tmp_ot + offs,
                 len2 * sizeof(Evas_Font_OT_Info));
           memcpy(props->info->glyph + offt, tmp_glyph + offs, len2 *
                 sizeof(Evas_Font_Glyph_Info));

           prev_pen_after = tmp_glyph[offs - 1].pen_after;
        }

      /* free old information */
      free(tmp_ot);
      free(tmp_glyph);
   }

   gl_itr = props->info->glyph + props->start;
   ot_itr = props->info->ot + props->start;
   infos = hb_buffer_get_glyph_infos(buffer, NULL);
   positions = hb_buffer_get_glyph_positions(buffer, NULL);

   /* If there were items prior current item (i.e. len1 > 0), then need to calc
    * from the last pen position */
   if (len1 > 0)
     {
        pen_after = gl_itr[-1].pen_after;
     }

   /* Update the item with new info */
   for (i = 0; i < (size_t)(glen); i++)
     {
        /* actually, this is wrong. Need to check clusters at the range
         * [0, props->start - 1] */
        ot_itr->source_cluster = infos->cluster;

        ot_itr->x_offset = positions->x_offset;
        ot_itr->y_offset = positions->y_offset;
        gl_itr->index = infos->codepoint;

        adv = EVAS_FONT_ROUND_26_6_TO_INT(positions->x_advance);
        pen_after += adv;
        gl_itr->pen_after = pen_after;

        infos++;
        ot_itr++;
        gl_itr++;
        positions++;
     }

   /* check if cluster/ligature at end of item if formed */

   /* update pen_after of rest of glyphs */
   if (len2 > 0)
     {
        int d = pen_after - prev_pen_after;
        /* rectify pen_after values */
        /* rectify cluster indices. The cluster of the next unaffected item
         * should be the same as the its position in text. */
        clust_diff = len - props->text_len;
        for (i = 0; i < (size_t)len2; i++)
          {
             /* rectify values */
             ot_itr->source_cluster += clust_diff;
             gl_itr->pen_after += d;

             gl_itr++;
             ot_itr++;
          }
     }
   hb_buffer_destroy(buffer);
   evas_common_font_int_use_trim();

   return EINA_FALSE;
}

/* Splits the text props of an item into two individual text props.
 * The provided text must be the text that is unchanged in the
 * [0,text_offset - 1] and [text_offset + text_len, len] ranges.
 * The first resultant part will end the info at the end of the props,
 * and the second part will start where the props ends, and end at what's left.
 */
EAPI Eina_Bool
evas_common_font_ot_hard_split_text_props(const Eina_Unicode *text,
      Evas_Text_Props *props, size_t len, int off, Evas_Text_Props *props2, Evas_Text_Props_Mode mode)
{
   Evas_Font_Glyph_Info *glyph, *old_glyph;
   Evas_Font_OT_Info *ot, *old_ot;
   hb_buffer_t *buffer;
   size_t i;
   int glen, added_info_len, info_len, old_info_len;
   int info_diff;
   int len1;
   int info_len2, tlen2;
   hb_glyph_position_t *positions;
   hb_glyph_info_t *infos;
   Evas_Font_Glyph_Info *gl_itr;
   Evas_Font_OT_Info *ot_itr;
   Evas_Coord pen_x = 0;


   old_info_len = props->info->len;

   buffer = hb_buffer_create();
   hb_buffer_set_unicode_funcs(buffer, _evas_common_font_ot_unicode_funcs_get());
   hb_buffer_set_language(buffer, hb_language_from_string(
            evas_common_language_from_locale_get(), -1));
   hb_buffer_set_script(buffer, _evas_script_to_harfbuzz[props->script]);
   hb_buffer_set_direction(buffer,
         (props->bidi_dir == EVAS_BIDI_DIRECTION_RTL) ?
         HB_DIRECTION_RTL : HB_DIRECTION_LTR);
   /* reshape the item part in the unicode string */
   hb_buffer_add_utf32(buffer, (const uint32_t *) text, len,
         props->text_offset, off); //offset and length of item in node's string
   _evas_common_font_ot_shape(buffer, props->font_instance, mode);

   glen = hb_buffer_get_length(buffer); //get glyph length of text
   /* truncate props info so it ends with the current item */
   props->info->len += glen - (props->start + props->len);
   props->len = glen;

   old_glyph = props->info->glyph;
   old_ot = props->info->ot;
   props->info->glyph = malloc(props->info->len * sizeof(Evas_Font_Glyph_Info));
   props->info->ot = malloc(props->info->len * sizeof(Evas_Font_Glyph_Info));
   positions = hb_buffer_get_glyph_positions(buffer, NULL);
   infos = hb_buffer_get_glyph_infos(buffer, NULL);

   /* copy all glyph info before this item */
   if (props->start > 0)
     {
        memcpy(props->info->glyph, old_glyph, props->start * sizeof(Evas_Font_Glyph_Info));
        memcpy(props->info->ot, old_ot, props->start * sizeof(Evas_Font_OT_Info));
     }

   gl_itr = props->info->glyph + props->start;
   ot_itr = props->info->ot + props->start;
   for (i = 0 ; i < props->len ; i++)
     {
        Evas_Coord adv;
        ot_itr->source_cluster = infos->cluster;
        ot_itr->x_offset = positions->x_offset;
        ot_itr->y_offset = positions->y_offset;
        gl_itr->index = infos->codepoint;
        adv = positions->x_advance;

        pen_x += adv;
        gl_itr->pen_after = EVAS_FONT_ROUND_26_6_TO_INT(pen_x);

        ot_itr++;
        gl_itr++;
        infos++;
        positions++;
     }
   hb_buffer_destroy(buffer);


   /* Second Part */
   tlen2 = len - off - props->text_offset;
   props2->info->len = old_info_len - (props->start + props->len);
   buffer = hb_buffer_create();
   hb_buffer_set_unicode_funcs(buffer, _evas_common_font_ot_unicode_funcs_get());
   hb_buffer_set_language(buffer, hb_language_from_string(
            evas_common_language_from_locale_get(), -1));
   hb_buffer_set_script(buffer, _evas_script_to_harfbuzz[props->script]);
   hb_buffer_set_direction(buffer,
         (props->bidi_dir == EVAS_BIDI_DIRECTION_RTL) ?
         HB_DIRECTION_RTL : HB_DIRECTION_LTR);
   /* reshape the item part in the unicode string */
   hb_buffer_add_utf32(buffer, (const uint32_t *) text, len,
         props->text_offset + off, tlen2); //offset and length of item in node's string
   _evas_common_font_ot_shape(buffer, props->font_instance, mode);

   props2->info->glyph = malloc(props2->info->len * sizeof(Evas_Font_Glyph_Info));
   props2->info->ot = malloc(props2->info->len * sizeof(Evas_Font_Glyph_Info));

   glen = hb_buffer_get_length(buffer); //get glyph length of text
   memcpy(props2->info->glyph, old_glyph + props->start + props->len, glen * sizeof(Evas_Font_Glyph_Info));
   memcpy(props2->info->ot, old_ot + props->start + props->len, glen * sizeof(Evas_Font_OT_Info));

   positions = hb_buffer_get_glyph_positions(buffer, NULL);
   infos = hb_buffer_get_glyph_infos(buffer, NULL);

   gl_itr = props2->info->glyph;
   ot_itr = props2->info->ot;
   for (i = 0 ; i < props->len ; i++)
     {
        Evas_Coord adv;
        ot_itr->source_cluster = infos->cluster;
        ot_itr->x_offset = positions->x_offset;
        ot_itr->y_offset = positions->y_offset;
        gl_itr->index = infos->codepoint;
        adv = positions->x_advance;

        pen_x += adv;
        gl_itr->pen_after = EVAS_FONT_ROUND_26_6_TO_INT(pen_x);

        ot_itr++;
        gl_itr++;
        infos++;
        positions++;
     }

   hb_buffer_destroy(buffer);

   free(old_glyph);
   free(old_ot);

   return EINA_TRUE;
}

#endif

