#include "common_funcs.h"

int _eolian_gen_log_dom = -1;

void
_template_fill(Eina_Strbuf *buf, const char* templ, const char* classname, const char *funcname, Eina_Bool reset)
{
   static char capobjclass[0xFF];
   static char lowobjclass[0xFF];
   static char capclass[0xFF];
   static char lowclass[0xFF];
   static char normclass[0xFF];
   static char capfunc[0xFF];
   static char eoprefix[0xFF];

   char *p;

   if (reset) eina_strbuf_reset(buf);
   if (templ) eina_strbuf_append(buf, templ);

   if (strcmp(classname, normclass))
     {
        //Fill cache
        strncpy(normclass, classname, sizeof(normclass) - 1);

        strncpy(capclass, classname, sizeof(capclass) - 1);
        p = capclass;
        eina_str_toupper(&p);

        strncpy(lowclass, classname, sizeof(lowclass) - 1);
        p = lowclass;
        eina_str_tolower(&p);

        Eina_Strbuf *classobj = eina_strbuf_new();
        eina_strbuf_append(classobj, classname);

        // More to exclusion list
        if (strcmp(classname, "Eo_Base") && strcmp(classname, "Elm_Widget"))
          eina_strbuf_replace(classobj, "_", "_obj_", 1);

        if (!strcmp(classname, "Evas_Object"))
          {
            eina_strbuf_reset(classobj);
            eina_strbuf_append(classobj, "Evas_Obj");
          }

        strncpy(capobjclass, eina_strbuf_string_get(classobj),
              sizeof(capobjclass) - 1);
        p = capobjclass;
        eina_str_toupper(&p);

        strncpy(lowobjclass, eina_strbuf_string_get(classobj),
              sizeof(lowobjclass) - 1);
        p = lowobjclass;
        eina_str_tolower(&p);

        strncpy(eoprefix, lowobjclass, sizeof(eoprefix) - 1);

        if (!strcmp(classname, "Elm_Widget"))
          strncpy(eoprefix, "elm_wdg", sizeof(eoprefix) - 1);

        eina_strbuf_free(classobj);
     }

   strncpy(capfunc, funcname, sizeof(capfunc) - 1);
   p = capfunc; eina_str_toupper(&p);

   eina_strbuf_replace_all(buf, "@#func", funcname);
   eina_strbuf_replace_all(buf, "@#FUNC", capfunc);
   eina_strbuf_replace_all(buf, "@#Class", classname);
   eina_strbuf_replace_all(buf, "@#class", lowclass);
   eina_strbuf_replace_all(buf, "@#CLASS", capclass);
   eina_strbuf_replace_all(buf, "@#OBJCLASS", capobjclass);
   eina_strbuf_replace_all(buf, "@#objclass", lowobjclass);
   eina_strbuf_replace_all(buf, "@#eoprefix", eoprefix);
}

char*
_nextline(char *str, unsigned int lines)
{
   if (!str) return NULL;

   char *ret = str;
   while (lines--)
     {
        ret= strchr(ret, '\n');
        if (ret) ret++;
        else return NULL;
     }
   return ret;
}

char*
_startline(char *str, char *pos)
{
   if (!str || !pos) return NULL;

   char *ret =  pos;
   while ((ret > str) && (*(ret-1)!='\n')) ret--;

   return ret;
}

char*
_source_desc_get(const char *str)
{
   Eina_Strbuf *part = eina_strbuf_new();
   if (str)
     {
        const char *p = strchr(str, '\n');
        size_t offs = (p) ? (size_t)(p - str) : strlen(str);
        eina_strbuf_append_n(part, str, offs);
        eina_strbuf_replace_all(part, "\\", "\\\\");
        eina_strbuf_replace_all(part, "\"", "\\\"");
     }
   char *ret = eina_strbuf_string_steal(part);
   eina_strbuf_free(part);
   return ret;
}
