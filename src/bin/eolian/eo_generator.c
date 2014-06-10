#include <Eina.h>
#include <string.h>

#include "Eolian.h"
#include "eo_generator.h"
#include "common_funcs.h"

static _eolian_class_vars class_env;

static const char
tmpl_dtor[] = "\
static void\n\
_gen_@#class_class_destructor(Eo_Class *klass)\n\
{\n\
   _@#class_class_destructor(klass);\n\
}\n\
\n\
";

static const char
tmpl_eo_ops_desc[] = "\
static Eo_Op_Description _@#class_op_desc[] = {@#list_op\n\
     EO_OP_SENTINEL\n\
};\n\n";

static const char
tmpl_events_desc[] = "\
static const Eo_Event_Description *_@#class_event_desc[] = {@#list_evdesc\n\
     NULL\n\
};\n\n";

static const char
tmpl_eo_src[] = "\
@#functions_body\
\n\
@#ctor_func\
@#dtor_func\
@#ops_desc\
@#events_desc\
static const Eo_Class_Description _@#class_class_desc = {\n\
     EO_VERSION,\n\
     \"@#Class\",\n\
     @#type_class,\n\
     @#eo_class_desc_ops,\n\
     @#Events_Desc,\n\
     @#SizeOfData,\n\
     @#ctor_name,\n\
     @#dtor_name\n\
};\n\
\n\
EO_DEFINE_CLASS(@#class_class_get, &_@#class_class_desc, @#list_inheritNULL);\
";

static const char
tmpl_eo_obj_header[] = "\
#define @#CLASS_CLASS @#class_class_get()\n\
\n\
const Eo_Class *@#class_class_get(void) EINA_CONST;\n\
\n\
";

static const char
tmpl_eo_funcdef_doxygen[] = "\
/**\n\
 *\n\
@#desc\n\
 *\n\
@#list_desc_param\
@#ret_desc\
 *\n\
 */\n";

static const char
tmpl_eo_pardesc[] =" * @param[%s] %s %s\n";

#if 0
static const char
tmpl_eo_retdesc[] =" * @return %s\n";
#endif

static Eina_Bool
eo_fundef_generate(const Eolian_Class class, Eolian_Function func, Eolian_Function_Type ftype, Eina_Strbuf *functext)
{
   _eolian_class_func_vars func_env;
   const char *str_dir[] = {"in", "out", "inout"};
   const Eina_List *l;
   void *data;
   char funcname[0xFF];
   char descname[0xFF];
   char *tmpstr = malloc(0x1FF);
   Eina_Bool var_as_ret = EINA_FALSE;
   const char *rettype = NULL;
   Eina_Bool ret_const = EINA_FALSE;
   Eolian_Function_Scope scope = eolian_function_scope_get(func);

   _class_func_env_create(class, eolian_function_name_get(func), ftype, &func_env);
   char *fsuffix = "";
   rettype = eolian_function_return_type_get(func, ftype);
   if (ftype == EOLIAN_PROP_GET)
     {
        fsuffix = "_get";
        if (!rettype)
          {
             l = eolian_parameters_list_get(func);
             if (eina_list_count(l) == 1)
               {
                  data = eina_list_data_get(l);
                  eolian_parameter_information_get((Eolian_Function_Parameter)data, NULL, &rettype, NULL, NULL);
                  var_as_ret = EINA_TRUE;
                  ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
               }
          }
     }
   if (ftype == EOLIAN_PROP_SET) fsuffix = "_set";

   sprintf (funcname, "%s%s", eolian_function_name_get(func), fsuffix);
   sprintf (descname, "comment%s", fsuffix);
   const char *funcdesc = eolian_function_description_get(func, descname);

   Eina_Strbuf *str_func = eina_strbuf_new();
   if (scope == EOLIAN_SCOPE_PROTECTED)
      eina_strbuf_append_printf(str_func, "#ifdef %s_PROTECTED\n", class_env.upper_classname);

   eina_strbuf_append(str_func, tmpl_eo_funcdef_doxygen);
   eina_strbuf_append_printf(str_func, "EOAPI @#rettype %s(@#full_params);\n", func_env.lower_eo_func);

   if (scope == EOLIAN_SCOPE_PROTECTED)
      eina_strbuf_append_printf(str_func, "#endif\n");
   eina_strbuf_append_printf(str_func, "\n");

   Eina_Strbuf *linedesc = eina_strbuf_new();
   eina_strbuf_append(linedesc, funcdesc ? funcdesc : "No description supplied.");
   if (eina_strbuf_length_get(linedesc))
     {
        eina_strbuf_replace_all(linedesc, "\n", "\n * ");
        eina_strbuf_replace_all(linedesc, " * \n", " *\n");
        eina_strbuf_prepend(linedesc, " * ");
     }
   else
     {
        eina_strbuf_append(linedesc, " *");
     }

   eina_strbuf_replace_all(str_func, "@#desc", eina_strbuf_string_get(linedesc));
   eina_strbuf_free(linedesc);

   Eina_Strbuf *str_par = eina_strbuf_new();
   Eina_Strbuf *str_pardesc = eina_strbuf_new();
   Eina_Strbuf *str_retdesc = eina_strbuf_new();
   Eina_Strbuf *str_typecheck = eina_strbuf_new();

   EINA_LIST_FOREACH(eolian_property_keys_list_get(func), l, data)
     {
        const char *pname;
        const char *ptype;
        const char *pdesc = NULL;
        eolian_parameter_information_get((Eolian_Function_Parameter)data, NULL, &ptype, &pname, &pdesc);

        eina_strbuf_append_printf(str_pardesc, tmpl_eo_pardesc, "in", pname, pdesc?pdesc:"No description supplied.");

        if (eina_strbuf_length_get(str_par)) eina_strbuf_append(str_par, ", ");
        eina_strbuf_append_printf(str_par, "%s %s", ptype, pname);
     }

   if (!var_as_ret)
     {
        EINA_LIST_FOREACH(eolian_parameters_list_get(func), l, data)
          {
             const char *pname;
             const char *ptype;
             const char *pdesc;
             Eina_Bool add_star = EINA_FALSE;
             Eolian_Parameter_Dir pdir;
             eolian_parameter_information_get((Eolian_Function_Parameter)data, &pdir, &ptype, &pname, &pdesc);
             Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
             if (ftype == EOLIAN_PROP_GET) {
                  add_star = EINA_TRUE;
                  pdir = EOLIAN_OUT_PARAM;
             }
             if (ftype == EOLIAN_PROP_SET) pdir = EOLIAN_IN_PARAM;
             if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM);
             Eina_Bool had_star = !!strchr(ptype, '*');

             const char *dir_str = str_dir[(int)pdir];

             eina_strbuf_append_printf(str_pardesc, tmpl_eo_pardesc, dir_str, pname, pdesc?pdesc:"No description supplied.");

             if (eina_strbuf_length_get(str_par)) eina_strbuf_append(str_par, ", ");
             eina_strbuf_append_printf(str_par, "%s%s%s%s%s",
                   is_const?"const ":"",
                   ptype, had_star?"":" ", add_star?"*":"", pname);

          }
     }

   tmpstr[0] = '\0';
   sprintf(tmpstr, "%s%s%s",
         ret_const ? "const " : "",
         rettype ? rettype : "void",
         rettype && strchr(rettype, '*')?"":" ");
   eina_strbuf_replace_all(str_func, "@#rettype", tmpstr);

   eina_strbuf_replace_all(str_func, "@#list_param", eina_strbuf_string_get(str_par));
   if (!eina_strbuf_length_get(str_par)) eina_strbuf_append(str_par, "void");
   eina_strbuf_replace_all(str_func, "@#full_params", eina_strbuf_string_get(str_par));
   eina_strbuf_replace_all(str_func, "@#list_desc_param", eina_strbuf_string_get(str_pardesc));
   eina_strbuf_replace_all(str_func, "@#ret_desc", eina_strbuf_string_get(str_retdesc));
   eina_strbuf_replace_all(str_func, "@#list_typecheck", eina_strbuf_string_get(str_typecheck));

   free(tmpstr);
   eina_strbuf_free(str_par);
   eina_strbuf_free(str_retdesc);
   eina_strbuf_free(str_pardesc);
   eina_strbuf_free(str_typecheck);

   eina_strbuf_append(functext, eina_strbuf_string_get(str_func));
   eina_strbuf_free(str_func);

   return EINA_TRUE;
}

Eina_Bool
eo_header_generate(const Eolian_Class class, Eina_Strbuf *buf)
{
   const Eolian_Function_Type ftype_order[] = {EOLIAN_CTOR, EOLIAN_PROPERTY, EOLIAN_METHOD};
   const Eina_List *itr;
   Eolian_Function fid;
   char *tmpstr = malloc(0x1FF);
   Eina_Strbuf * str_hdr = eina_strbuf_new();

   const char *desc = eolian_class_description_get(class);
   _class_env_create(class, NULL, &class_env);

   if (desc)
     {
        Eina_Strbuf *linedesc = eina_strbuf_new();
        eina_strbuf_append(linedesc, "/**\n");
        eina_strbuf_append(linedesc, desc);
        eina_strbuf_replace_all(linedesc, "\n", "\n * ");
        eina_strbuf_append(linedesc, "\n */\n");
        eina_strbuf_replace_all(linedesc, " * \n", " *\n"); /* Remove trailing whitespaces */
        eina_strbuf_append(buf, eina_strbuf_string_get(linedesc));
        eina_strbuf_free(linedesc);
     }

   _template_fill(str_hdr, tmpl_eo_obj_header, class, NULL, NULL, EINA_TRUE);

   eina_strbuf_replace_all(str_hdr, "@#EOPREFIX", class_env.upper_eo_prefix);
   eina_strbuf_replace_all(str_hdr, "@#eoprefix", class_env.lower_eo_prefix);

   Eina_Strbuf *str_ev = eina_strbuf_new();
   Eina_Strbuf *str_extrn_ev = eina_strbuf_new();
   Eina_Strbuf *tmpbuf = eina_strbuf_new();

   Eolian_Event event;
   EINA_LIST_FOREACH(eolian_class_events_list_get(class), itr, event)
     {
        const char *evname = NULL;
        const char *evdesc = NULL;
        eolian_class_event_information_get(event, &evname, NULL, &evdesc);

        if (!evdesc) evdesc = "No description";
        eina_strbuf_reset(tmpbuf);
        eina_strbuf_append(tmpbuf, evdesc);
        eina_strbuf_replace_all(tmpbuf, "\n", "\n * ");
        eina_strbuf_prepend(tmpbuf," * ");
        eina_strbuf_append_printf(str_ev, "\n/**\n%s\n */\n", eina_strbuf_string_get(tmpbuf));

        eina_strbuf_reset(tmpbuf);
        eina_strbuf_append_printf(tmpbuf, "%s_EVENT_%s", class_env.upper_classname, evname);
        eina_strbuf_replace_all(tmpbuf, ",", "_");
        char* s = (char *)eina_strbuf_string_get(tmpbuf);
        eina_str_toupper(&s);
        eina_strbuf_append_printf(str_ev, "#define %s (&(_%s))\n", s, s);
        eina_strbuf_append_printf(str_extrn_ev, "EOAPI extern const Eo_Event_Description _%s;\n", s);
     }

   int i;
   for (i = 0; i < 3; i++)
      EINA_LIST_FOREACH(eolian_class_functions_list_get(class, ftype_order[i]), itr, fid)
        {
           const Eolian_Function_Type ftype = eolian_function_type_get(fid);
           Eina_Bool prop_read = (ftype == EOLIAN_PROPERTY || ftype == EOLIAN_PROP_GET ) ? EINA_TRUE : EINA_FALSE ;
           Eina_Bool prop_write = (ftype == EOLIAN_PROPERTY || ftype == EOLIAN_PROP_SET ) ? EINA_TRUE : EINA_FALSE ;

           if (!prop_read && !prop_write)
             {
                eo_fundef_generate(class, fid, EOLIAN_UNRESOLVED, str_hdr);
             }
           if (prop_write)
             {
                eo_fundef_generate(class, fid, EOLIAN_PROP_SET, str_hdr);
             }
           if (prop_read)
             {
                eo_fundef_generate(class, fid, EOLIAN_PROP_GET, str_hdr);
             }
        }

   eina_strbuf_append(str_hdr, eina_strbuf_string_get(str_extrn_ev));
   eina_strbuf_append(str_hdr, eina_strbuf_string_get(str_ev));

   eina_strbuf_append(buf, eina_strbuf_string_get(str_hdr));

   free(tmpstr);
   eina_strbuf_free(str_ev);
   eina_strbuf_free(str_extrn_ev);
   eina_strbuf_free(tmpbuf);
   eina_strbuf_free(str_hdr);

   return EINA_TRUE;
}

static Eina_Bool
eo_bind_func_generate(const Eolian_Class class, Eolian_Function funcid, Eolian_Function_Type ftype, Eina_Strbuf *buf, _eolian_class_vars *impl_env)
{
   _eolian_class_func_vars func_env;
   const char *suffix = "";
   Eina_Bool var_as_ret = EINA_FALSE;
   const char *rettype = NULL;
   const char *retname = NULL;
   Eina_Bool ret_const = EINA_FALSE;
   Eina_Bool add_star = EINA_FALSE;

   Eina_Bool need_implementation = EINA_TRUE;
   if (!impl_env && eolian_function_is_virtual_pure(funcid, ftype)) need_implementation = EINA_FALSE;

   Eina_Strbuf *fbody = eina_strbuf_new();
   Eina_Strbuf *va_args = eina_strbuf_new();
   Eina_Strbuf *params = eina_strbuf_new(); /* only variables names */
   Eina_Strbuf *full_params = eina_strbuf_new(); /* variables types + names */

   rettype = eolian_function_return_type_get(funcid, ftype);
   retname = "ret";
   if (ftype == EOLIAN_PROP_GET)
     {
        suffix = "_get";
        add_star = EINA_TRUE;
        if (!rettype)
          {
             const Eina_List *l = eolian_parameters_list_get(funcid);
             if (eina_list_count(l) == 1)
               {
                  void* data = eina_list_data_get(l);
                  eolian_parameter_information_get((Eolian_Function_Parameter)data, NULL, &rettype, &retname, NULL);
                  var_as_ret = EINA_TRUE;
                  ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
               }
          }
     }
   if (ftype == EOLIAN_PROP_SET)
     {
        suffix = "_set";
     }

   const Eina_List *l;
   void *data;

   EINA_LIST_FOREACH(eolian_property_keys_list_get(funcid), l, data)
     {
        const char *pname;
        const char *ptype;
        eolian_parameter_information_get((Eolian_Function_Parameter)data, NULL, &ptype, &pname, NULL);
        Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
        if (eina_strbuf_length_get(params)) eina_strbuf_append(params, ", ");
        eina_strbuf_append_printf(params, "%s", pname);
        eina_strbuf_append_printf(full_params, ", %s%s %s",
              is_const?"const ":"",
              ptype, pname);
     }
   if (!var_as_ret)
     {
        EINA_LIST_FOREACH(eolian_parameters_list_get(funcid), l, data)
          {
             const char *pname;
             const char *ptype;
             Eolian_Parameter_Dir pdir;
             eolian_parameter_information_get((Eolian_Function_Parameter)data, &pdir, &ptype, &pname, NULL);
             Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
             Eina_Bool had_star = !!strchr(ptype, '*');
             if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM);
             if (eina_strbuf_length_get(params)) eina_strbuf_append(params, ", ");
             eina_strbuf_append_printf(params, "%s", pname);
             eina_strbuf_append_printf(full_params, ", %s%s%s%s%s",
                   is_const?"const ":"",
                   ptype, had_star?"":" ", add_star?"*":"", pname);
          }
     }

   if (need_implementation)
     {
        Eina_Strbuf *ret_param = eina_strbuf_new();
        eina_strbuf_append_printf(fbody, "\n");
        eina_strbuf_append_printf(fbody, "%s%s _%s%s%s_%s%s(Eo *obj, @#Datatype_Data *pd@#full_params);\n\n",
              ret_const?"const ":"", rettype?rettype:"void",
              class_env.lower_classname,
              impl_env?"_":"",
              impl_env?impl_env->lower_classname:"",
              eolian_function_name_get(funcid), suffix);

        eina_strbuf_free(ret_param);
     }
   if (!impl_env)
     {
        Eina_Strbuf *eo_func_decl = eina_strbuf_new();
        Eina_Bool has_params =
           eina_list_count(eolian_property_keys_list_get(funcid)) ||
           (!var_as_ret && eina_list_count(eolian_parameters_list_get(funcid)) != 0);
        Eina_Bool ret_is_void = (!rettype || !strcmp(rettype, "void"));
        _class_func_env_create(class, eolian_function_name_get(funcid), ftype, &func_env);
        eina_strbuf_append_printf(eo_func_decl,
              "EOAPI EO_%sFUNC_BODY%s(%s",
              ret_is_void?"VOID_":"", has_params?"V":"",
              func_env.lower_eo_func);
        if (!ret_is_void)
          {
             const char *dflt_ret_val =
                eolian_function_return_dflt_value_get(funcid, ftype);
             eina_strbuf_append_printf(eo_func_decl, ", %s%s, %s",
                   ret_const ? "const " : "", rettype,
                   dflt_ret_val?dflt_ret_val:"0");

          }
        if (has_params)
          {
             eina_strbuf_append_printf(eo_func_decl, ", EO_FUNC_CALL(%s)%s",
                   eina_strbuf_string_get(params),
                   eina_strbuf_string_get(full_params));
          }
        eina_strbuf_append_printf(eo_func_decl, ");");
        eina_strbuf_append_printf(fbody, "%s\n", eina_strbuf_string_get(eo_func_decl));
        eina_strbuf_free(eo_func_decl);
     }

   if (need_implementation)
     {
        eina_strbuf_replace_all(fbody, "@#full_params", eina_strbuf_string_get(full_params));
        const char *data_type = eolian_class_data_type_get(class);
        if (data_type && !strcmp(data_type, "null"))
           eina_strbuf_replace_all(fbody, "@#Datatype_Data", "void");
        else
          {
             if (data_type) eina_strbuf_replace_all(fbody, "@#Datatype_Data", data_type);
             else eina_strbuf_replace_all(fbody, "@#Datatype", class_env.full_classname);
          }

        if (!data_type || !strcmp(data_type, "null"))
           eina_strbuf_replace_all(fbody, "@#Datatype", class_env.full_classname);
        else
           eina_strbuf_replace_all(fbody, "@#Datatype_Data", data_type);
     }
   eina_strbuf_append(buf, eina_strbuf_string_get(fbody));

   eina_strbuf_free(va_args);
   eina_strbuf_free(full_params);
   eina_strbuf_free(params);
   eina_strbuf_free(fbody);
   return EINA_TRUE;
}

static Eina_Bool
eo_op_desc_generate(const Eolian_Class class, Eolian_Function fid, Eolian_Function_Type ftype,
      const char *desc, Eina_Strbuf *buf)
{
   _eolian_class_func_vars func_env;
   const char *funcname = eolian_function_name_get(fid);
   const char *suffix = "";

   eina_strbuf_reset(buf);
   _class_func_env_create(class, funcname, ftype, &func_env);
   if (ftype == EOLIAN_PROP_GET) suffix = "_get";
   if (ftype == EOLIAN_PROP_SET) suffix = "_set";
   Eina_Bool is_virtual_pure = eolian_function_is_virtual_pure(fid, ftype);
   eina_strbuf_append_printf(buf, "\n     EO_OP_FUNC(%s, ", func_env.lower_eo_func);
   if (!is_virtual_pure)
      eina_strbuf_append_printf(buf, "_%s_%s%s, \"%s\"),", class_env.lower_classname, funcname, suffix, desc);
   else
      eina_strbuf_append_printf(buf, "NULL, \"%s\"),", desc);

   return EINA_TRUE;
}

static Eina_Bool
eo_source_beginning_generate(const Eolian_Class class, Eina_Strbuf *buf)
{
   const Eina_List *itr;

   Eina_Strbuf *tmpbuf = eina_strbuf_new();
   Eina_Strbuf *str_ev = eina_strbuf_new();

   Eolian_Event event;
   EINA_LIST_FOREACH(eolian_class_events_list_get(class), itr, event)
     {
        const char *evname;
        const char *evdesc;
        char *evdesc_line1;
        char *p;

        eina_strbuf_reset(str_ev);
        eolian_class_event_information_get(event, &evname, NULL, &evdesc);
        evdesc_line1 = _source_desc_get(evdesc);
        eina_strbuf_append_printf(str_ev, "%s_EVENT_%s", class_env.upper_classname, evname);
        p = (char *)eina_strbuf_string_get(str_ev);
        eina_str_toupper(&p);
        eina_strbuf_replace_all(str_ev, ",", "_");

        eina_strbuf_append_printf(tmpbuf,
                                  "EOAPI const Eo_Event_Description _%s =\n   EO_EVENT_DESCRIPTION(\"%s\", \"%s\");\n",
                                  eina_strbuf_string_get(str_ev), evname, evdesc_line1);
        free(evdesc_line1);
     }

   eina_strbuf_append(buf, eina_strbuf_string_get(tmpbuf));

   eina_strbuf_free(str_ev);
   eina_strbuf_free(tmpbuf);
   return EINA_TRUE;
}

static Eina_Bool
eo_source_end_generate(const Eolian_Class class, Eina_Strbuf *buf)
{
   Eina_Bool ret = EINA_FALSE;
   const Eina_List *itr;
   Eolian_Function fn;

   const char *str_classtype = NULL;
   switch(eolian_class_type_get(class))
    {
      case EOLIAN_CLASS_REGULAR:
        str_classtype = "EO_CLASS_TYPE_REGULAR";
        break;
      case EOLIAN_CLASS_ABSTRACT:
        str_classtype = "EO_CLASS_TYPE_REGULAR_NO_INSTANT";
        break;
      case EOLIAN_CLASS_MIXIN:
        str_classtype = "EO_CLASS_TYPE_MIXIN";
        break;
      case EOLIAN_CLASS_INTERFACE:
        str_classtype = "EO_CLASS_TYPE_INTERFACE";
        break;
      default:
        break;
    }

   if (!str_classtype)
     {
        ERR ("Unknown class type for class %s !", class_env.full_classname);
        return EINA_FALSE;
     }

   Eina_Strbuf *str_end = eina_strbuf_new();
   Eina_Strbuf *tmpbuf = eina_strbuf_new();
   Eina_Strbuf *str_op = eina_strbuf_new();
   Eina_Strbuf *str_bodyf = eina_strbuf_new();
   Eina_Strbuf *str_ev = eina_strbuf_new();

   _template_fill(str_end, tmpl_eo_src, class, NULL, NULL, EINA_TRUE);

   eina_strbuf_replace_all(str_end, "@#type_class", str_classtype);
   eina_strbuf_replace_all(str_end, "@#EOPREFIX", class_env.upper_eo_prefix);
   eina_strbuf_replace_all(str_end, "@#eoprefix", class_env.lower_eo_prefix);

   eina_strbuf_reset(tmpbuf);
   eina_strbuf_replace_all(str_end, "@#ctor_func", eina_strbuf_string_get(tmpbuf));

   eina_strbuf_reset(tmpbuf);
   if (eolian_class_ctor_enable_get(class))
      _template_fill(tmpbuf, "_@#class_class_constructor", class, NULL, NULL, EINA_TRUE);
   else
      eina_strbuf_append_printf(tmpbuf, "NULL");
   eina_strbuf_replace_all(str_end, "@#ctor_name", eina_strbuf_string_get(tmpbuf));

   eina_strbuf_reset(tmpbuf);
   if (eolian_class_dtor_enable_get(class))
     {
        eina_strbuf_replace_all(str_end, "@#dtor_func", eina_strbuf_string_get(tmpbuf));
        eina_strbuf_reset(tmpbuf);
        _template_fill(tmpbuf, "_@#class_class_destructor", class, NULL, NULL, EINA_TRUE);
        eina_strbuf_replace_all(str_end, "@#dtor_name", eina_strbuf_string_get(tmpbuf));
     }
   else
     {
        eina_strbuf_replace_all(str_end, "@#dtor_func", "");
        eina_strbuf_replace_all(str_end, "@#dtor_name", "NULL");
     }

   eina_strbuf_reset(tmpbuf);

   //Implements - TODO one generate func def for all
   Eolian_Implement impl_desc;
   EINA_LIST_FOREACH(eolian_class_implements_list_get(class), itr, impl_desc)
     {
        _eolian_class_vars impl_env;
        char implname[0xFF];
        Eolian_Class impl_class = NULL;
        Eolian_Function_Type ftype;
        Eolian_Function fnid = NULL;
        const char *funcname = NULL;
        char *tp = implname;

        if (eolian_implement_information_get(impl_desc, &impl_class, &fnid, &ftype))
          {
             _class_env_create(impl_class, NULL, &impl_env);
             funcname = eolian_function_name_get(fnid);

             sprintf(implname, "%s_%s", class_env.full_classname, impl_env.full_classname);
             eina_str_tolower(&tp);
          }

        if (!fnid)
          {
             ERR ("Failed to generate implementation of %s - missing form super class",
                   eolian_implement_full_name_get(impl_desc));
             goto end;
          }

        switch (ftype)
          {
           case EOLIAN_PROP_SET: case EOLIAN_PROP_GET: case EOLIAN_PROPERTY:
              if (ftype != EOLIAN_PROP_GET)
                {
                   eina_strbuf_append_printf(str_op, "\n     EO_OP_FUNC_OVERRIDE(%s_%s_set, _%s_%s_set),",
                         impl_env.lower_eo_prefix, funcname, implname, funcname);
                   eo_bind_func_generate(class, fnid, EOLIAN_PROP_SET, str_bodyf, &impl_env);
                }

              if (ftype != EOLIAN_PROP_SET)
                {
                   eina_strbuf_append_printf(str_op, "\n     EO_OP_FUNC_OVERRIDE(%s_%s_get, _%s_%s_get),",
                         impl_env.lower_eo_prefix, funcname, implname, funcname);
                   eo_bind_func_generate(class, fnid, EOLIAN_PROP_GET, str_bodyf, &impl_env);
                }
              break;
           default:
              eina_strbuf_append_printf(str_op, "\n     EO_OP_FUNC_OVERRIDE(%s_%s, _%s_%s),",
                    impl_env.lower_eo_prefix, funcname, implname, funcname);
              eo_bind_func_generate(class, fnid, ftype, str_bodyf, &impl_env);
              break;
          }
     }

   //Constructors
   EINA_LIST_FOREACH(eolian_class_functions_list_get(class, EOLIAN_CTOR), itr, fn)
     {
        char *desc = _source_desc_get(eolian_function_description_get(fn, "comment"));
        eo_op_desc_generate(class, fn, EOLIAN_CTOR, desc, tmpbuf);
        eina_strbuf_append(str_op, eina_strbuf_string_get(tmpbuf));
        free(desc);

        eo_bind_func_generate(class, fn, EOLIAN_UNRESOLVED, str_bodyf, NULL);
     }

   //Properties
   EINA_LIST_FOREACH(eolian_class_functions_list_get(class, EOLIAN_PROPERTY), itr, fn)
     {
        const char *funcname = eolian_function_name_get(fn);
        const Eolian_Function_Type ftype = eolian_function_type_get(fn);
        char tmpstr[0xFF];

        Eina_Bool prop_read = ( ftype == EOLIAN_PROP_SET ) ? EINA_FALSE : EINA_TRUE;
        Eina_Bool prop_write = ( ftype == EOLIAN_PROP_GET ) ? EINA_FALSE : EINA_TRUE;

        if (prop_write)
          {
             char *desc = _source_desc_get(eolian_function_description_get(fn, "comment_set"));

             sprintf(tmpstr, "%s_set", funcname);
             eo_op_desc_generate(class, fn, EOLIAN_PROP_SET, desc, tmpbuf);
             eina_strbuf_append(str_op, eina_strbuf_string_get(tmpbuf));
             free(desc);
          }
        if (prop_read)
          {
             char *desc = _source_desc_get(eolian_function_description_get(fn, "comment_get"));

             sprintf(tmpstr, "%s_get", funcname);
             eo_op_desc_generate(class, fn, EOLIAN_PROP_GET, desc, tmpbuf);
             eina_strbuf_append(str_op, eina_strbuf_string_get(tmpbuf));
             free(desc);
          }
     }

   //Methods
   EINA_LIST_FOREACH(eolian_class_functions_list_get(class, EOLIAN_METHOD), itr, fn)
     {
        char *desc = _source_desc_get(eolian_function_description_get(fn, "comment"));
        eo_op_desc_generate(class, fn, EOLIAN_METHOD, desc, tmpbuf);
        eina_strbuf_append(str_op, eina_strbuf_string_get(tmpbuf));
        free(desc);
     }

   Eolian_Event event;
   EINA_LIST_FOREACH(eolian_class_events_list_get(class), itr, event)
     {
        const char *evname;
        char *p;

        eina_strbuf_reset(tmpbuf);
        eolian_class_event_information_get(event, &evname, NULL, NULL);
        eina_strbuf_append_printf(tmpbuf, "%s_EVENT_%s", class_env.upper_classname, evname);
        p = (char *)eina_strbuf_string_get(tmpbuf);
        eina_str_toupper(&p);
        eina_strbuf_replace_all(tmpbuf, ",", "_");
        eina_strbuf_append_printf(str_ev, "\n     %s,", eina_strbuf_string_get(tmpbuf));
     }

   eina_strbuf_reset(tmpbuf);
   if (eina_strbuf_length_get(str_ev))
     {
        Eina_Strbuf *events_desc = eina_strbuf_new();
        _template_fill(events_desc, tmpl_events_desc, class, NULL, NULL, EINA_TRUE);
        eina_strbuf_replace_all(events_desc, "@#list_evdesc", eina_strbuf_string_get(str_ev));
        eina_strbuf_replace_all(str_end, "@#events_desc", eina_strbuf_string_get(events_desc));
        eina_strbuf_free(events_desc);
        eina_strbuf_append_printf(tmpbuf, "_%s_event_desc", class_env.lower_classname);
     }
   else
     {
        eina_strbuf_append_printf(tmpbuf, "NULL");
        eina_strbuf_replace_all(str_end, "@#events_desc", "");
     }
   eina_strbuf_replace_all(str_end, "@#Events_Desc", eina_strbuf_string_get(tmpbuf));

   const char *inherit_name;
   eina_strbuf_reset(tmpbuf);
   EINA_LIST_FOREACH(eolian_class_inherits_list_get(class), itr, inherit_name)
     {
        Eolian_Class inherit_class = eolian_class_find_by_name(inherit_name);
        _eolian_class_vars inherit_env;
        _class_env_create(inherit_class, NULL, &inherit_env);
        eina_strbuf_append_printf(tmpbuf, "%s_CLASS, ", inherit_env.upper_classname);
     }

   if (eina_strbuf_length_get(tmpbuf) == 0) eina_strbuf_append(tmpbuf, "NULL, ");
   eina_strbuf_replace_all(str_end, "@#list_inherit", eina_strbuf_string_get(tmpbuf));

   eina_strbuf_reset(tmpbuf);
   if (eina_strbuf_length_get(str_op))
     {
        Eina_Strbuf *ops_desc = eina_strbuf_new();
        _template_fill(ops_desc, tmpl_eo_ops_desc, class, NULL, NULL, EINA_TRUE);
        eina_strbuf_replace_all(ops_desc, "@#list_op", eina_strbuf_string_get(str_op));
        eina_strbuf_replace_all(str_end, "@#ops_desc", eina_strbuf_string_get(ops_desc));
        eina_strbuf_free(ops_desc);
        _template_fill(tmpbuf,
              "EO_CLASS_DESCRIPTION_OPS(_@#class_op_desc)",
              class, NULL, NULL, EINA_TRUE);
     }
   else
     {
        eina_strbuf_replace_all(str_end, "@#ops_desc", "");
        eina_strbuf_append_printf(tmpbuf, "EO_CLASS_DESCRIPTION_NOOPS()");
     }

   eina_strbuf_replace_all(str_end, "@#functions_body", eina_strbuf_string_get(str_bodyf));
   eina_strbuf_replace_all(str_end, "@#eo_class_desc_ops", eina_strbuf_string_get(tmpbuf));

   const char *data_type = eolian_class_data_type_get(class);
   if (data_type && !strcmp(data_type, "null"))
      eina_strbuf_replace_all(str_end, "@#SizeOfData", "0");
   else
     {
        Eina_Strbuf *sizeofbuf = eina_strbuf_new();
        eina_strbuf_append_printf(sizeofbuf, "sizeof(%s%s)",
              data_type?data_type:class_env.full_classname,
              data_type?"":"_Data");
        eina_strbuf_replace_all(str_end, "@#SizeOfData", eina_strbuf_string_get(sizeofbuf));
        eina_strbuf_free(sizeofbuf);
     }
   eina_strbuf_append(buf, eina_strbuf_string_get(str_end));

   ret = EINA_TRUE;
end:
   eina_strbuf_free(tmpbuf);
   eina_strbuf_free(str_op);
   eina_strbuf_free(str_bodyf);
   eina_strbuf_free(str_end);
   eina_strbuf_free(str_ev);

   return ret;
}

Eina_Bool
eo_source_generate(const Eolian_Class class, Eina_Strbuf *buf)
{
   Eina_Bool ret = EINA_FALSE;
   const Eina_List *itr;
   Eolian_Function fn;

   Eina_Strbuf *str_bodyf = eina_strbuf_new();

   _class_env_create(class, NULL, &class_env);

   if (!eo_source_beginning_generate(class, buf)) goto end;

   //Properties
   EINA_LIST_FOREACH(eolian_class_functions_list_get(class, EOLIAN_PROPERTY), itr, fn)
     {
        const Eolian_Function_Type ftype = eolian_function_type_get(fn);

        Eina_Bool prop_read = ( ftype == EOLIAN_PROP_SET ) ? EINA_FALSE : EINA_TRUE;
        Eina_Bool prop_write = ( ftype == EOLIAN_PROP_GET ) ? EINA_FALSE : EINA_TRUE;

        if (prop_write)
          {
             if (!eo_bind_func_generate(class, fn, EOLIAN_PROP_SET, str_bodyf, NULL)) goto end;
          }
        if (prop_read)
          {
             if (!eo_bind_func_generate(class, fn, EOLIAN_PROP_GET, str_bodyf, NULL)) goto end;
          }
     }

   //Methods
   EINA_LIST_FOREACH(eolian_class_functions_list_get(class, EOLIAN_METHOD), itr, fn)
     {
        if (!eo_bind_func_generate(class, fn, EOLIAN_UNRESOLVED, str_bodyf, NULL)) goto end;
     }

   eina_strbuf_append(buf, eina_strbuf_string_get(str_bodyf));
   eina_strbuf_reset(str_bodyf);

   if (!eo_source_end_generate(class, buf)) goto end;

   ret = EINA_TRUE;
end:
   eina_strbuf_free(str_bodyf);
   return ret;
}

