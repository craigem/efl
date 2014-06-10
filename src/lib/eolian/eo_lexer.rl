#include <stdio.h>
#include <stdlib.h>
#include <Eina.h>

#include "Eolian.h"
#
#include "eo_lexer.h"
#include "eolian_database.h"

static int _eo_tokenizer_log_dom = -1;
#ifdef CRITICAL
#undef CRITICAL
#endif
#define CRITICAL(...) EINA_LOG_DOM_CRIT(_eo_tokenizer_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eo_tokenizer_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eo_tokenizer_log_dom, __VA_ARGS__)

#define INF_ENABLED EINA_FALSE
#ifdef INF
#undef INF
#endif
#define INF(...) if (INF_ENABLED) EINA_LOG_DOM_INFO(_eo_tokenizer_log_dom, __VA_ARGS__)

#define DBG_ENABLED EINA_FALSE
#ifdef DBG
#undef DBG
#endif
#define DBG(...) if (DBG_ENABLED) EINA_LOG_DOM_DBG(_eo_tokenizer_log_dom, __VA_ARGS__)

#define FUNC_PUBLIC 0
#define FUNC_PROTECTED 1

static int _init_counter = 0;

int
eo_tokenizer_init()
{
   if (!_init_counter)
     {
        eina_init();
        eina_log_color_disable_set(EINA_FALSE);
        _eo_tokenizer_log_dom = eina_log_domain_register("eo_toknz", EINA_COLOR_CYAN);
     }
   return _init_counter++;
}

int
eo_tokenizer_shutdown()
{
   if (_init_counter <= 0) return 0;
   _init_counter--;
   if (!_init_counter)
     {
        eina_log_domain_unregister(_eo_tokenizer_log_dom);
        _eo_tokenizer_log_dom = -1;
        eina_shutdown();
     }
   return _init_counter;
}

static void
_eo_tokenizer_abort(Eo_Tokenizer *toknz,
                    const char *file, const char* fct, int line,
                    const char *fmt, ...)
{
   va_list ap;
   va_start (ap, fmt);
   eina_log_vprint(_eo_tokenizer_log_dom, EINA_LOG_LEVEL_ERR,
                   file, fct, line, fmt, ap);
   va_end(ap);
   fprintf(stderr, "File:%s\n toknz[%d] n:%d l:%d p:%d pe:%d ts:%s te:%s act:%d\n",
          toknz->source,
          toknz->cs, toknz->current_nesting, toknz->current_line,
          (int)(toknz->p - toknz->buf), (int)(toknz->pe - toknz->buf),
          toknz->ts, toknz->te, toknz->act);
   exit(EXIT_FAILURE);
}
#define ABORT(toknz, ...) \
   _eo_tokenizer_abort(toknz, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

static void _eo_tokenizer_normalize_buf(char *buf)
{
   int c;
   char *s, *d;
   Eina_Bool in_space = EINA_TRUE;
   Eina_Bool in_newline = EINA_FALSE;

   /* ' '+ -> ' '
    * '\n' ' '* '*' ' '* -> '\n'
    */
   for (s = buf, d = buf; *s != '\0'; s++)
     {
        c = *s;
        *d = c;

        if (!in_space || (c != ' '))
          d++;

        if (c == ' ')
          in_space = EINA_TRUE;
        else
          in_space = EINA_FALSE;

        if (c == '\n')
          {
             in_newline = EINA_TRUE;
             in_space = EINA_TRUE;
          }
        else if (in_newline && c == '*' )
          {
             in_space = EINA_TRUE;
             in_newline = EINA_FALSE;
             d--;
          }
     }
   /* ' '+$ -> $ */
   d--;
   while (*d == ' ') d--;
   d++;
   if (d < buf) return;
   *d = '\0';
}

static const char*
_eo_tokenizer_token_get(Eo_Tokenizer *toknz, char *p)
{
   if (toknz->saved.tok == NULL) ABORT(toknz, "toknz->saved.tok is NULL");
   char d[BUFSIZE];
   int l = (p - toknz->saved.tok);
   memcpy(d, toknz->saved.tok, l);
   d[l] = '\0';
   _eo_tokenizer_normalize_buf(d);
   toknz->saved.tok = NULL;
   DBG("token : >%s<", d);
   return eina_stringshare_add(d);
}

static Eo_Class_Def*
_eo_tokenizer_class_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Class_Def *kls = calloc(1, sizeof(Eo_Class_Def));
   if (kls == NULL) ABORT(toknz, "calloc Eo_Class_Def failure");

   kls->name = _eo_tokenizer_token_get(toknz, p);

   return kls;
}

static Eo_Type_Def*
_eo_tokenizer_type_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Type_Def *def = calloc(1, sizeof(Eo_Type_Def));
   if (def == NULL) ABORT(toknz, "calloc Eo_Type_Def failure");

   def->type = _eo_tokenizer_token_get(toknz, p);

   return def;
}

static Eo_Property_Def*
_eo_tokenizer_property_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Property_Def *prop = calloc(1, sizeof(Eo_Property_Def));
   if (prop == NULL) ABORT(toknz, "calloc Eo_Property_Def failure");

   prop->name = _eo_tokenizer_token_get(toknz, p);
   prop->scope = toknz->tmp.fscope;
   toknz->tmp.fscope = FUNC_PUBLIC;

   return prop;
}

static Eo_Method_Def*
_eo_tokenizer_method_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Method_Def *meth = calloc(1, sizeof(Eo_Method_Def));
   if (meth == NULL) ABORT(toknz, "calloc Eo_Method_Def failure");

   meth->name = _eo_tokenizer_token_get(toknz, p);
   meth->scope = toknz->tmp.fscope;
   toknz->tmp.fscope = FUNC_PUBLIC;

   return meth;
}

static int
_eo_tokenizer_scope_get(Eo_Tokenizer *toknz, EINA_UNUSED char *p)
{
   if (!strncmp(toknz->saved.tok, "protected ", 10))
     return FUNC_PROTECTED;

   return FUNC_PUBLIC;
}

static Eo_Param_Def*
_eo_tokenizer_param_get(Eo_Tokenizer *toknz, char *p)
{
   char *s;

   Eo_Param_Def *param = calloc(1, sizeof(Eo_Param_Def));
   if (param == NULL) ABORT(toknz, "calloc Eo_Param_Def failure");

   /* The next code part tries to identify the different tags of the
      parameter.
      First, we set the ';' to '\0', to search only inside this section.
      We then strstr the different tags and if found, we update the internal
      flag and clear the zone of the text. In this way, during the
      determination of the type/variable, we will not be disturbed by the
      flags.
      We have to put back the ';' at the end.
    */
   *p = '\0';
   s = strstr(toknz->saved.tok, "@nonull");
   if (s)
     {
        param->nonull = EINA_TRUE;
        memset(s, ' ', 7);
     }
   *p = ';';
   s = p - 1; /* Don't look at the character ';' */
   /* Remove any space between the param name and ';'/@nonull
    * This loop fixes the case where "char *name ;" becomes the type of the param.
    */
   while (*s == ' ') s--;
   for (; s >= toknz->saved.tok; s--)
     {
        if ((*s == ' ') || (*s == '*'))
          break;
     }

   if (s == toknz->saved.tok)
     ABORT(toknz, "wrong parameter: %s", _eo_tokenizer_token_get(toknz, p));
   s++;

   param->way = PARAM_IN;
   if (strncmp(toknz->saved.tok, "@in ", 4) == 0)
     {
        toknz->saved.tok += 3;
        param->way = PARAM_IN;
     }
   else if (strncmp(toknz->saved.tok, "@out ", 5) == 0)
     {
        toknz->saved.tok += 4;
        param->way = PARAM_OUT;
     }
   else if (strncmp(toknz->saved.tok, "@inout ", 7) == 0)
     {
        toknz->saved.tok += 6;
        param->way = PARAM_INOUT;
     }

   param->type = _eo_tokenizer_token_get(toknz, s);

   toknz->saved.tok = s;
   param->name = _eo_tokenizer_token_get(toknz, p);

   return param;
}

static Eo_Ret_Def*
_eo_tokenizer_return_get(Eo_Tokenizer *toknz, char *p)
{
   char *s;

   Eo_Ret_Def *ret = calloc(1, sizeof(Eo_Ret_Def));
   if (ret == NULL) ABORT(toknz, "calloc Eo_Ret_Def failure");

   *p = '\0';
   s = strstr(toknz->saved.tok, "@warn_unused");
   if (s)
     {
        ret->warn_unused = EINA_TRUE;
        memset(s, ' ', 12);
     }
   s = strchr(toknz->saved.tok, '(');
   if (s)
     {
        char *ret_val;
        char *end = strchr(s, ')');
        if (!end)
           ABORT(toknz, "wrong syntax (missing ')'): %s",
                 _eo_tokenizer_token_get(toknz, p));
        /* Current values in s and end have to be changed to ' ' to not disturb the next steps (type extraction) */
        *s++ = ' ';
        while (*s == ' ') s++;
        *end-- = ' ';
        while (end > s && *end == ' ') end--;
        if (end < s)
           ABORT(toknz, "empty default return value: %s",
                 _eo_tokenizer_token_get(toknz, p));
        ret_val = malloc(end - s + 2); /* string + '\0' */
        memcpy(ret_val, s, end - s + 1);
        ret_val[end - s + 1] = '\0';
        ret->dflt_ret_val = ret_val;
        memset(s, ' ', end - s + 1);
     }
   *p = ';';
   s = p - 1; /* Don't look at the character ';' */
   /* Remove any space between the param name and ';'
    * This loop fixes the case where "char *name ;" becomes the type of the param.
    */
   while (*s == ' ') s--;

   if (s == toknz->saved.tok)
     ABORT(toknz, "wrong parameter: %s", _eo_tokenizer_token_get(toknz, p));
   s++;

   ret->type = _eo_tokenizer_token_get(toknz, s);

   return ret;
}

static Eo_Accessor_Param*
_eo_tokenizer_accessor_param_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Accessor_Param *param = calloc(1, sizeof(Eo_Accessor_Param));
   if (param == NULL) ABORT(toknz, "calloc Eo_Accessor_Param failure");

   /* Remove the colon and spaces - we just need the param name */
   while (*p != ':') p--;
   p--;
   while (*p == ' ') p--;
   param->name = _eo_tokenizer_token_get(toknz, p + 1);

   return param;
}

static Eo_Accessor_Def *
_eo_tokenizer_accessor_get(Eo_Tokenizer *toknz, Eo_Accessor_Type type)
{
   Eo_Accessor_Def *accessor = calloc(1, sizeof(Eo_Accessor_Def));
   if (accessor == NULL) ABORT(toknz, "calloc Eo_Accessor_Def failure");

   accessor->type = type;

   return accessor;
}

static Eo_Event_Def*
_eo_tokenizer_event_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Event_Def *sgn = calloc(1, sizeof(Eo_Event_Def));
   if (sgn == NULL) ABORT(toknz, "calloc Eo_Event_Def failure");

   sgn->name = _eo_tokenizer_token_get(toknz, p);

   return sgn;
}

static Eo_Implement_Def*
_eo_tokenizer_implement_get(Eo_Tokenizer *toknz, char *p)
{
   Eo_Implement_Def *impl = calloc(1, sizeof(Eo_Implement_Def));
   if (impl == NULL) ABORT(toknz, "calloc Eo_Implement_Def failure");

   impl->meth_name = _eo_tokenizer_token_get(toknz, p);

   return impl;
}

%%{
   machine common;

   access toknz->;
   variable p toknz->p;
   variable pe toknz->pe;
   variable eof toknz->eof;

   action inc_line {
      toknz->current_line += 1;
      DBG("inc[%d] %d", toknz->cs, toknz->current_line);
   }

   action save_line {
      toknz->saved.line = toknz->current_line;
      DBG("save line[%d] %d", toknz->cs, toknz->current_line);
   }

   action save_fpc {
      toknz->saved.tok = fpc;
      DBG("save token[%d] %p %c", toknz->cs, fpc, *fpc);
   }

   action move_ts {
      DBG("move ts %d chars forward", (int)(fpc - toknz->ts));
      toknz->ts = fpc;
   }

   action show_comment {
      DBG("comment[%d] line%03d:%03d", toknz->cs,
          toknz->saved.line, toknz->current_line);
   }

   action show_ignore {
      DBG("ignore[%d] line:%d", toknz->cs, toknz->current_line);
   }

   action show_error {
      DBG("error[%d]", toknz->cs);
      char *s, *d;
      char buf[BUFSIZE];
      for (s = fpc, d = buf; (s <= toknz->pe); s++)
        {
           if ((*s == '\r') || (*s == '\n'))
             break;
           *d++ = *s;
        }
      *d = '\0';
      ERR("error n:%d l:%d c:'%c': %s",
          toknz->current_nesting, toknz->current_line, *fpc, buf);
      toknz->cs = eo_tokenizer_error;
      fbreak;  /* necessary to stop scanners */
   }

   cr                = '\n';
   cr_neg            = [^\n];
   ws                = [ \t\r];
   newline           = cr @inc_line;
   ignore            = (0x00..0x20 - cr) | newline;

   alnum_u           = alnum | '_';
   alpha_u           = alpha | '_';
   ident             = alpha+ >save_fpc (alnum | '_' )*;
   event             = alpha+ >save_fpc (alnum | '_' | ',' )*;
   class_meth        = alpha+ >save_fpc (alnum | '_' | '::' )*;
   class_name        = alpha+ >save_fpc (alnum | '_' | '::' )*;

   eo_comment        = "/*@" ignore* ('@' | alnum_u) >save_fpc ( any | cr @inc_line )* :>> "*/";
   c_comment         = "/*" ( any | cr @inc_line )* :>> "*/";
   cpp_comment       = "//" (any - cr )* newline;
   comment           = ( c_comment | cpp_comment ) > save_line;

   end_statement     = ';';
   begin_def         = '{';
   end_def           = '}' end_statement?;
   begin_list        = '(';
   end_list          = ')';
   list_separator    = ',';
   colon             = ':';

   # chars allowed on the return line.
   return_char       = (alnum_u | '*' | ws | '@' | '(' | ')' | '.' | '-' | '<' | '>');
   scope             = ('public' | 'protected');
   scope_def         = scope >save_fpc ws+ %move_ts;
   func_name         = ((alnum (alnum | '_')?)+ - scope) >save_fpc;
}%%

%%{
   machine eo_tokenizer;
   include common;

   write data;

###### TOKENIZE ACCESSOR

   action end_accessor_comment {
      if (!toknz->tmp.accessor) ABORT(toknz, "No accessor!!!");
      if (toknz->tmp.accessor->comment != NULL)
        ABORT(toknz, "accessor has already a comment");
      toknz->tmp.accessor->comment = _eo_tokenizer_token_get(toknz, fpc-1);
      INF("        %s", toknz->tmp.accessor->comment);
   }

   action end_accessor_return {
      if (!toknz->tmp.accessor) ABORT(toknz, "No accessor!!!");
      if (toknz->tmp.accessor->ret != NULL)
        ABORT(toknz, "accessor has already a return type");
      toknz->tmp.accessor->ret = _eo_tokenizer_return_get(toknz, fpc);
   }

   action end_accessor_rettype_comment {
      if (!toknz->tmp.accessor) ABORT(toknz, "No accessor!!!");
      if (!toknz->tmp.accessor->ret) ABORT(toknz, "No ret!!!");
      if (toknz->tmp.accessor->ret->comment != NULL)
        ABORT(toknz, "accessor return type has already a comment");
      toknz->tmp.accessor->ret->comment = _eo_tokenizer_token_get(toknz, fpc-2);
      INF("        %s", toknz->tmp.accessor->ret->comment);
   }

   action end_accessor_legacy {
      if (!toknz->tmp.accessor) ABORT(toknz, "No accessor!!!");
      toknz->tmp.accessor->legacy = _eo_tokenizer_token_get(toknz, fpc);
   }

   action end_accessor {
      INF("      }");
      if (!toknz->tmp.prop) ABORT(toknz, "No prop!!!");
      toknz->tmp.prop->accessors = eina_list_append(toknz->tmp.prop->accessors, toknz->tmp.accessor);
      toknz->tmp.accessor = NULL;
      toknz->current_nesting--;
      fgoto tokenize_property;
   }

   action begin_param_desc {
      toknz->tmp.accessor_param = _eo_tokenizer_accessor_param_get(toknz, fpc);
   }

   action end_param_desc {
      if (!toknz->tmp.accessor_param)
         ABORT(toknz, "No accessor param!!!");
      toknz->tmp.accessor_param->attrs = _eo_tokenizer_token_get(toknz, fpc);
      toknz->tmp.accessor->params =
         eina_list_append(toknz->tmp.accessor->params, toknz->tmp.accessor_param);
      toknz->tmp.accessor_param = NULL;
   }

   rettype_comment = ws* eo_comment %end_accessor_rettype_comment;
   rettype = 'return' ws+ return_char >save_fpc return_char+ %end_accessor_return end_statement rettype_comment?;

   legacy = 'legacy' ws+ ident %end_accessor_legacy end_statement;

   param_desc = ident ws* colon %begin_param_desc ws* alpha+ >save_fpc (alnum_u | list_separator | ws)* %end_param_desc end_statement;

   tokenize_accessor := |*
      ignore+;    #=> show_ignore;
      eo_comment  => end_accessor_comment;
      comment     => show_comment;
      rettype;
      legacy;
      param_desc;
      end_def     => end_accessor;
      any         => show_error;
      *|;

###### TOKENIZE PARAMS

   action end_param_comment {
      const char *c = _eo_tokenizer_token_get(toknz, fpc-2);
      if (toknz->tmp.param == NULL)
        ABORT(toknz, "no parameter set to associate this comment to: %s", c);
      toknz->tmp.param->comment = c;
      toknz->tmp.param = NULL;
   }

   action end_param {
      toknz->tmp.param = _eo_tokenizer_param_get(toknz, fpc);
      if (toknz->tmp.params)
        *(toknz->tmp.params) = eina_list_append(*(toknz->tmp.params), toknz->tmp.param);
      else
        ABORT(toknz, "got a param but there is no property nor method waiting for it");
      INF("        %s : %s", toknz->tmp.param->name, toknz->tmp.param->type);
   }

   action end_params {
      INF("      }");
      toknz->tmp.param = NULL;
      toknz->current_nesting--;
      if (toknz->tmp.prop)
        fgoto tokenize_property;
      else if (toknz->tmp.meth)
        fgoto tokenize_method;
      else
        ABORT(toknz, "leaving tokenize_params but there is no property nor method pending");
   }

   param_comment = ws* eo_comment %end_param_comment;
   param = ('@'|alpha+) >save_fpc (alnum_u | '*' | '@' | '<' | '>' | ws )+ %end_param end_statement param_comment?;

   tokenize_params := |*
      ignore+;    #=> show_ignore;
      comment     => show_comment;
      param;
      end_def     => end_params;
      any         => show_error;
      *|;

###### TOKENIZE PROPERTY

   action begin_property_get {
      INF("      get {");
      toknz->tmp.accessor = _eo_tokenizer_accessor_get(toknz, GETTER);
      toknz->current_nesting++;
      fgoto tokenize_accessor;
   }

   action begin_property_set {
      INF("      set {");
      toknz->tmp.accessor = _eo_tokenizer_accessor_get(toknz, SETTER);
      toknz->current_nesting++;
      fgoto tokenize_accessor;
   }

   action begin_property_keys {
      INF("      keys {");
      toknz->current_nesting++;
      toknz->tmp.params = &(toknz->tmp.prop->keys);
      fgoto tokenize_params;
   }

   action begin_property_values {
      INF("      values {");
      toknz->current_nesting++;
      toknz->tmp.params = &(toknz->tmp.prop->values);
      fgoto tokenize_params;
   }

   action end_property {
      if (!toknz->tmp.prop) ABORT(toknz, "No property!!!");
      if (eina_list_count(toknz->tmp.prop->accessors) == 0)
        WRN("property '%s' has no accessors.", toknz->tmp.prop->name);
      INF("    }");
      toknz->tmp.kls->properties = eina_list_append(toknz->tmp.kls->properties, toknz->tmp.prop);
      toknz->tmp.prop = NULL;
      toknz->tmp.fscope = FUNC_PUBLIC;
      toknz->current_nesting--;
      fgoto tokenize_properties;
   }

   action end_prop_as_ctor{
      if (!toknz->tmp.prop) ABORT(toknz, "No property!!!");
      toknz->tmp.prop->is_ctor = EINA_TRUE;
      INF("        constructor");
   }

   prop_get = 'get' ignore* begin_def;
   prop_set = 'set' ignore* begin_def;
   prop_keys = 'keys' ignore* begin_def;
   prop_values = 'values' ignore* begin_def;

   prop_as_ctor = 'constructor' %end_prop_as_ctor end_statement;

   tokenize_property := |*
      ignore+;    #=> show_ignore;
      comment     => show_comment;
      prop_get    => begin_property_get;
      prop_set    => begin_property_set;
      prop_keys   => begin_property_keys;
      prop_values => begin_property_values;
      prop_as_ctor;
      end_def     => end_property;
      any         => show_error;
      *|;

###### TOKENIZE PROPERTIES

   action begin_property {
      if (!toknz->tmp.prop) ABORT(toknz, "No property!!!");
      INF("    %s {", toknz->tmp.prop->name);
      toknz->current_nesting++;
      fgoto tokenize_property;
   }

   action end_property_name {
      if (toknz->tmp.prop != NULL)
        ABORT(toknz, "there is a pending property definition %s", toknz->tmp.prop->name);
      toknz->tmp.prop = _eo_tokenizer_property_get(toknz, fpc);
   }

   action end_property_scope {
      toknz->tmp.fscope = _eo_tokenizer_scope_get(toknz, fpc);
   }

   action end_properties {
      INF("  }");
      toknz->current_nesting--;
      fgoto tokenize_class;
   }

   begin_property = (scope_def %end_property_scope)? func_name %end_property_name ignore* begin_def;

   tokenize_properties := |*
      ignore+;       #=> show_ignore;
      comment        => show_comment;
      begin_property => begin_property;
      end_def        => end_properties;
      any            => show_error;
      *|;

###### TOKENIZE METHOD

   action end_method_comment {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      if (toknz->tmp.meth->comment != NULL)
        ABORT(toknz, "method has already a comment");
      toknz->tmp.meth->comment = _eo_tokenizer_token_get(toknz, fpc-1);
      INF("        %s", toknz->tmp.meth->comment);
   }

   action begin_method_params {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      INF("      params {");
      toknz->current_nesting++;
      toknz->tmp.params = &(toknz->tmp.meth->params);
      fgoto tokenize_params;
   }

   action end_method_rettype {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      if (toknz->tmp.meth->ret != NULL)
        ABORT(toknz, "method '%s' has already a return type", toknz->tmp.meth->name);
      toknz->tmp.meth->ret = _eo_tokenizer_return_get(toknz, fpc);
   }

   action end_method_rettype_comment {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      if (toknz->tmp.meth->ret == NULL) ABORT(toknz, "No ret!!!");
      if (toknz->tmp.meth->ret->comment != NULL)
        ABORT(toknz, "method '%s' return type has already a comment", toknz->tmp.meth->name);
      toknz->tmp.meth->ret->comment = _eo_tokenizer_token_get(toknz, fpc-2);
      INF("        %s", toknz->tmp.meth->ret->comment);
   }

   action end_method_legacy {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      toknz->tmp.meth->legacy = _eo_tokenizer_token_get(toknz, fpc);
   }

   action end_method_obj_const{
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      toknz->tmp.meth->obj_const = EINA_TRUE;
      INF("        obj const");
   }

   action end_method_as_ctor{
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      toknz->tmp.meth->is_ctor = EINA_TRUE;
      INF("        constructor");
   }

   action end_method {
      Eina_List **l = NULL;
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      INF("    }");
      switch (toknz->current_methods_type) {
        case METH_CONSTRUCTOR:
          l = &toknz->tmp.kls->constructors;
          break;
        case METH_REGULAR:
          l = &toknz->tmp.kls->methods;
          break;
        default:
          ABORT(toknz, "unknown method type %d", toknz->current_methods_type);
      }
      toknz->tmp.meth->type = toknz->current_methods_type;
      *l = eina_list_append(*l, toknz->tmp.meth);
      toknz->tmp.meth = NULL;
      toknz->tmp.fscope = FUNC_PUBLIC;
      toknz->current_nesting--;
      fgoto tokenize_methods;
   }


   meth_params = 'params' ignore* begin_def;
   meth_legacy = 'legacy' ws+ ident %end_method_legacy end_statement;

   meth_rettype_comment = ws* eo_comment %end_method_rettype_comment;
   meth_rettype = 'return' ws+ return_char >save_fpc return_char+ %end_method_rettype end_statement meth_rettype_comment?;

   meth_obj_const = 'const' %end_method_obj_const end_statement;

   meth_as_ctor = 'constructor' %end_method_as_ctor end_statement;

   tokenize_method := |*
      ignore+;    #=> show_ignore;
      eo_comment  => end_method_comment;
      comment     => show_comment;
      meth_params => begin_method_params;
      meth_rettype;
      meth_legacy;
      meth_obj_const;
      meth_as_ctor;
      end_def     => end_method;
      any         => show_error;
      *|;

###### TOKENIZE METHODS

   action begin_method {
      if (!toknz->tmp.meth) ABORT(toknz, "No method!!!");
      INF("    %s {", toknz->tmp.meth->name);
      toknz->current_nesting++;
      fgoto tokenize_method;
   }

   action end_method_name {
      if (toknz->tmp.meth != NULL)
        ABORT(toknz, "there is a pending method definition %s", toknz->tmp.meth->name);
      toknz->tmp.meth = _eo_tokenizer_method_get(toknz, fpc);
   }

   action end_method_scope {
      toknz->tmp.fscope = _eo_tokenizer_scope_get(toknz, fpc);
   }

   action end_methods {
      INF("  }");
      toknz->current_methods_type = METH_TYPE_LAST;
      toknz->current_nesting--;
      fgoto tokenize_class;
   }

   begin_method = (scope_def %end_method_scope)? func_name %end_method_name ignore* begin_def;

   tokenize_methods := |*
      ignore+;       #=> show_ignore;
      comment        => show_comment;
      begin_method   => begin_method;
      end_def        => end_methods;
      any            => show_error;
      *|;

###### TOKENIZE CLASS

   action end_class_comment {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      if (toknz->tmp.kls->comment != NULL)
        ABORT(toknz, "class %s has already a comment", toknz->tmp.kls->name);
      toknz->tmp.kls->comment = _eo_tokenizer_token_get(toknz, fpc-1);
   }

   action end_str_item{
      const char *base = _eo_tokenizer_token_get(toknz, fpc);
      toknz->tmp.str_items = eina_list_append(toknz->tmp.str_items, base);
   }

   action end_inherits {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      toknz->tmp.kls->inherits = toknz->tmp.str_items;
      toknz->tmp.str_items = NULL;
   }

   action end_implements {
   }

   action end_events {
   }

   action begin_constructors {
      INF("  constructors {");
      toknz->current_methods_type = METH_CONSTRUCTOR;
      toknz->tmp.fscope = FUNC_PUBLIC;
      toknz->current_nesting++;
      fgoto tokenize_methods;
   }

   action begin_properties {
      INF("  properties {");
      toknz->tmp.fscope = FUNC_PUBLIC;
      toknz->current_nesting++;
      fgoto tokenize_properties;
   }

   action begin_methods {
      INF("  begin methods");
      toknz->current_methods_type = METH_REGULAR;
      toknz->tmp.fscope = FUNC_PUBLIC;
      toknz->current_nesting++;
      fgoto tokenize_methods;
   }

   action end_class {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      INF("end class: %s", toknz->tmp.kls->name);
      toknz->classes = eina_list_append(toknz->classes, toknz->tmp.kls);
      toknz->tmp.kls = NULL;
      toknz->current_nesting--;
      fgoto main;
   }

   action end_event_name {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      toknz->tmp.event = _eo_tokenizer_event_get(toknz, fpc);
      toknz->tmp.kls->events = eina_list_append(toknz->tmp.kls->events, toknz->tmp.event);
   }

   action end_event_type {
      if (!toknz->tmp.event) ABORT(toknz, "No event!!!");
      if (toknz->tmp.event->type != NULL)
        ABORT(toknz, "event %s has already a type %s", toknz->tmp.event->name, toknz->tmp.event->type);
      toknz->tmp.event->type = _eo_tokenizer_token_get(toknz, fpc-1);
   }

   action end_event_comment {
      if (!toknz->tmp.event) ABORT(toknz, "No event!!!");
      if (toknz->tmp.event->comment != NULL)
        ABORT(toknz, "event %s has already a comment", toknz->tmp.event->name);
      toknz->tmp.event->comment = _eo_tokenizer_token_get(toknz, fpc-2);
      toknz->tmp.event = NULL;
   }

   action end_legacy_prefix {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      if (toknz->tmp.kls->legacy_prefix != NULL)
        ABORT(toknz, "A legacy prefix has already been given");
      toknz->tmp.kls->legacy_prefix = _eo_tokenizer_token_get(toknz, fpc);
   }

   legacy_prefix = 'legacy_prefix' ignore* colon ignore* ident %end_legacy_prefix end_statement ignore*;

   action end_eo_prefix {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      if (toknz->tmp.kls->eo_prefix != NULL)
        ABORT(toknz, "An Eo prefix has already been given");
      toknz->tmp.kls->eo_prefix = _eo_tokenizer_token_get(toknz, fpc);
   }

   eo_prefix = 'eo_prefix' ignore* colon ignore* ident %end_eo_prefix end_statement ignore*;

   action end_data_type{
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      if (toknz->tmp.kls->data_type != NULL)
        ABORT(toknz, "A data type has already been given");
      toknz->tmp.kls->data_type = _eo_tokenizer_token_get(toknz, fpc);
   }

   data_type = 'data' ignore* colon ignore* ident %end_data_type end_statement ignore*;

   class_it = class_name %end_str_item ignore*;
   class_it_next = list_separator ignore* class_it;
   inherits = begin_list (class_it class_it_next*)? end_list %end_inherits;

   action impl_meth_store {
        if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
        toknz->tmp.impl = _eo_tokenizer_implement_get(toknz, fpc);
        toknz->tmp.kls->implements = eina_list_append(toknz->tmp.kls->implements, toknz->tmp.impl);
   }

   impl_it = class_meth %impl_meth_store ignore* end_statement ignore*;
# implements { ... }
   implements = 'implements' ignore* begin_def ignore* impl_it* end_def;

   event_comment = ws* eo_comment %end_event_comment;
   event_type = begin_list ws* alpha_u >save_fpc (alnum_u | '*' | ws )* ws* end_list %end_event_type;
   event_it = event %end_event_name ws* event_type? ignore* end_statement event_comment? ignore* comment* ignore*;
   events = 'events' ignore* begin_def ignore* event_it* end_def;

   constructors = 'constructors' ignore* begin_def;
   properties = 'properties' ignore* begin_def;
   methods = 'methods' ignore* begin_def;

   tokenize_class := |*
      ignore+;       #=> show_ignore;
      eo_comment     => end_class_comment;
      comment        => show_comment;
      legacy_prefix;
      eo_prefix;
      data_type;
      implements     => end_implements;
      events        => end_events;
      constructors   => begin_constructors;
      properties     => begin_properties;
      methods        => begin_methods;
      end_def        => end_class;
      any            => show_error;
      *|;

###### TOP LEVEL

   action begin_class {
      if (!toknz->tmp.kls) ABORT(toknz, "No class!!!");
      INF("begin class: %s", toknz->tmp.kls->name);
      toknz->current_nesting++;
      fgoto tokenize_class;
   }

   action class_type_set_to_class {
      toknz->tmp.kls_type = EOLIAN_CLASS_REGULAR;
   }
   action class_type_set_to_abstract {
      toknz->tmp.kls_type = EOLIAN_CLASS_ABSTRACT;
   }
   action class_type_set_to_mixin {
      toknz->tmp.kls_type = EOLIAN_CLASS_MIXIN;
   }
   action class_type_set_to_interface {
      toknz->tmp.kls_type = EOLIAN_CLASS_INTERFACE;
   }

   action end_class_name {
      if (toknz->tmp.kls != NULL)
        ABORT(toknz, "there is a pending class definition %s", toknz->tmp.kls->name);
      toknz->tmp.kls = _eo_tokenizer_class_get(toknz, fpc);
      toknz->tmp.kls->type = toknz->tmp.kls_type;
   }

   begin_class = (
         "class" %class_type_set_to_class |
         "mixin" %class_type_set_to_mixin |
         "abstract" %class_type_set_to_abstract |
         "interface" %class_type_set_to_interface) ws+ class_name %end_class_name ws* inherits? ignore* begin_def;

   action end_typedef_alias {
      toknz->tmp.typedef_alias = _eo_tokenizer_token_get(toknz, fpc);
   }

   action end_typedef_type{
      if (toknz->tmp.typedef_alias == NULL)
        ABORT(toknz, "No typedef");
      toknz->tmp.type_def = _eo_tokenizer_type_get(toknz, fpc);
      toknz->tmp.type_def->alias = toknz->tmp.typedef_alias;
      toknz->typedefs = eina_list_append(toknz->typedefs, toknz->tmp.type_def);
   }

   typedef_type = ('@'|alpha+) >save_fpc (alnum_u | '*' | '@' | '<' | '>' | ws )+;
   typedef_alias = ident %end_typedef_alias;
   begin_typedef = "type" ws+ typedef_alias ws* colon ws* typedef_type %end_typedef_type end_statement;

   main := |*
      ignore+;    #=> show_ignore;
      comment     => show_comment;
      begin_class => begin_class;
      begin_typedef;
      any         => show_error;
   *|;

}%%

Eina_Bool
eo_tokenizer_walk(Eo_Tokenizer *toknz, const char *source)
{
   INF("tokenize %s...", source);
   toknz->source = eina_stringshare_add(source);

   FILE *stream;
   Eina_Bool ret = EINA_TRUE;

   int done = 0;
   int have = 0;
   int offset = 0;

   stream = fopen(toknz->source, "rb");
   if (!stream)
     {
        ERR("unable to read in %s", toknz->source);
        return EINA_FALSE;
     }

   %% write init;

   while (!done)
     {
        int len;
        int space;

        toknz->p = toknz->buf + have;
        space = BUFSIZE - have;

        if (space == 0)
          {
             fclose(stream);
             ABORT(toknz, "out of buffer space");
          }

        len = fread(toknz->p, 1, space, stream);
        if (len == 0) break;
        toknz->pe = toknz->p + len;

        if (len < space)
          {
             toknz->eof = toknz->pe;
             done = 1;
          }

        %% write exec;

        if ( toknz->cs == %%{ write error; }%% )
          {
             ERR("%s: wrong termination", source);
             ret = EINA_FALSE;
             break;
          }

        if ( toknz->ts == 0 )
          have = 0;
        else
          {
             DBG("move data and pointers before buffer feed");
             have = toknz->pe - toknz->ts;
             offset = toknz->ts - toknz->buf;
             memmove(toknz->buf, toknz->ts, have);
             toknz->te -= offset;
             toknz->ts = toknz->buf;
          }

        if (toknz->saved.tok != NULL)
          {
             if ((have == 0) || ((toknz->saved.tok - offset) < toknz->buf))
               {
                  WRN("reset lost saved token %p", toknz->saved.tok);
                  toknz->saved.tok = NULL;
               }
             else
               toknz->saved.tok -= offset;
          }
     }

   fclose(stream);

   return ret;
}

static Eina_Bool
eo_tokenizer_mem_walk(Eo_Tokenizer *toknz, const char *source, char *buffer, unsigned int len)
{
   INF("tokenize %s...", source);
   toknz->source = eina_stringshare_add(source);

   Eina_Bool ret = EINA_TRUE;

   %% write init;

   toknz->p = buffer;

   toknz->pe = toknz->p + len;

   toknz->eof = toknz->pe;

   %% write exec;

   if ( toknz->cs == %%{ write error; }%% )
     {
        ERR("%s: wrong termination", source);
        ret = EINA_FALSE;
     }

   return ret;
}

Eo_Tokenizer*
eo_tokenizer_get(void)
{
   Eo_Tokenizer *toknz = calloc(1, sizeof(Eo_Tokenizer));
   if (!toknz) return NULL;

   toknz->ts = NULL;
   toknz->te = NULL;
   /* toknz->top = 0; */
   toknz->source = NULL;
   toknz->max_nesting = 10;
   toknz->current_line = 1;
   toknz->current_nesting = 0;
   toknz->current_methods_type = METH_TYPE_LAST;
   toknz->saved.tok = NULL;
   toknz->saved.line = 0;
   toknz->classes = NULL;
   toknz->typedefs = NULL;

   return toknz;
}

static char *_accessor_type_str[ACCESSOR_TYPE_LAST] = { "setter", "getter" };
static char *_param_way_str[PARAM_WAY_LAST] = { "IN", "OUT", "INOUT" };

void
eo_tokenizer_dump(Eo_Tokenizer *toknz)
{
   const char *s;
   Eina_List *k, *l, *m;

   Eo_Class_Def *kls;
   Eo_Property_Def *prop;
   Eo_Method_Def *meth;
   Eo_Param_Def *param;
   Eo_Accessor_Def *accessor;
   Eo_Event_Def *sgn;
   /* Eo_Ret_Def *ret; */

   EINA_LIST_FOREACH(toknz->classes, k, kls)
     {
        printf("Class: %s (%s)\n",
               kls->name, (kls->comment ? kls->comment : "-"));
        printf("  inherits from :");
        EINA_LIST_FOREACH(kls->inherits, l, s)
           printf(" %s", s);
        printf("\n");
        printf("  implements:");
        EINA_LIST_FOREACH(kls->implements, l, s)
           printf(" %s", s);
        printf("\n");
        printf("  events:\n");
        EINA_LIST_FOREACH(kls->events, l, sgn)
           printf("    %s <%s> (%s)\n", sgn->name, sgn->type, sgn->comment);

        EINA_LIST_FOREACH(kls->constructors, l, meth)
          {
             printf("  constructors: %s\n", meth->name);
             if (meth->ret)
                printf("    return: %s (%s)\n", meth->ret->type, meth->ret->comment);
             printf("    legacy : %s\n", meth->legacy);
             EINA_LIST_FOREACH(meth->params, m, param)
               {
                  printf("    param: %s %s : %s (%s)\n",
                         _param_way_str[param->way], param->name,
                         param->type, param->comment);
               }
          }

        EINA_LIST_FOREACH(kls->properties, l, prop)
          {
             printf("  property: %s\n", prop->name);
             EINA_LIST_FOREACH(prop->keys, m, param)
               {
                  printf("    key: %s : %s (%s)\n",
                         param->name, param->type, param->comment);
               }
             EINA_LIST_FOREACH(prop->values, m, param)
               {
                  printf("    value: %s : %s (%s)\n",
                         param->name, param->type, param->comment);
               }
             EINA_LIST_FOREACH(prop->accessors, m, accessor)
               {
                  printf("    accessor: %s : %s (%s)\n",
                         (accessor->ret?accessor->ret->type:""),
                         _accessor_type_str[accessor->type],
                         accessor->comment);
                  printf("      legacy : %s\n", accessor->legacy);
               }
             printf("    is_ctor : %s\n", prop->is_ctor?"true":"false");
          }

        EINA_LIST_FOREACH(kls->methods, l, meth)
          {
             printf("  method: %s\n", meth->name);
             if (meth->ret)
                printf("    return: %s (%s)\n", meth->ret->type, meth->ret->comment);
             printf("    legacy : %s\n", meth->legacy);
             printf("    obj_const : %s\n", meth->obj_const?"true":"false");
             printf("    is_ctor : %s\n", meth->is_ctor?"true":"false");
             EINA_LIST_FOREACH(meth->params, m, param)
               {
                  printf("    param: %s %s : %s (%s)\n",
                         _param_way_str[param->way], param->name,
                         param->type, param->comment);
               }
          }

     }

}

static Eina_Inlist *
_types_extract(const char *buf, int len)
{
   const char *save_buf = buf;
   Eolian_Type types = NULL;
   long depth = 0;
   char *tmp_type = malloc(2 * len + 1);

   while (len > 0)
     {
        char *d = tmp_type;
        Eina_Bool end_type = EINA_FALSE;
        Eina_Bool is_own = EINA_FALSE;
        char c;
        Eina_Bool in_spaces = EINA_TRUE, in_stars = EINA_FALSE;
        while (len > 0 && !end_type)
          {
             switch (c = *buf++)
               {
                /* @own */
                case '@':
                     {
                        if (!strncmp(buf, "own ", 4))
                          {
                             is_own = EINA_TRUE;
                             buf += 4; len -= 4;
                          }
                        break;
                     }
                /* if '*', we have to add a space. We set in_spaces to true in
                 * case spaces are between stars, to be sure we remove them.
                 */
                case '*':
                     {
                        if (!in_stars && !in_spaces)
                          {
                             *d++ = ' ';
                             in_stars = EINA_TRUE;
                             in_spaces = EINA_TRUE;
                          }
                        *d++ = c;
                        break;
                     }
                /* Only the first space is inserted. */
                case ' ':
                     {
                        if (!in_spaces) *d++ = c;
                        in_spaces = EINA_TRUE;
                        break;
                     }
                case '<':
                     {
                        if (depth < 0)
                          {
                             ERR("%s: Cannot reopen < after >", save_buf);
                             goto error;
                          }
                        depth++;
                        end_type = EINA_TRUE;
                        break;
                     }
                case '>':
                     {
                        if (depth == 0)
                          {
                             ERR("%s: Too much >", save_buf);
                             goto error;
                          }
                        if (depth > 0 && d == tmp_type)
                          {
                             ERR("%s: empty type inside <>", save_buf);
                             goto error;
                          }
                        if (depth > 0) depth *= -1;
                        depth++;
                        end_type = EINA_TRUE;
                        break;
                     }
                default:
                     {
                        *d++ = c;
                        in_spaces = EINA_FALSE;
                        in_stars = EINA_FALSE;
                     }
               }
             len--;
          }
        if (d != tmp_type)
          {
             *d = '\0';
             types = database_type_append(types, tmp_type, is_own);
          }
     }
   if (depth)
     {
        ERR("%s: < and > are not well used.", save_buf);
        goto error;
     }
   goto success;
error:
   database_type_del(types);
   types = NULL;
success:
   free(tmp_type);
   return types;
}

Eina_Bool
eo_tokenizer_database_fill(const char *filename)
{
   Eina_Bool ret = EINA_FALSE;
   const char *s;
   Eina_List *k, *l, *m;

   Eo_Class_Def *kls;
   Eo_Type_Def *type_def;
   Eo_Property_Def *prop;
   Eo_Method_Def *meth;
   Eo_Param_Def *param;
   Eo_Accessor_Def *accessor;
   Eo_Event_Def *event;
   Eo_Implement_Def *impl;

   FILE *stream = NULL;
   char *buffer = NULL;

   Eo_Tokenizer *toknz = eo_tokenizer_get();
   if (!toknz)
     {
        ERR("can't create eo_tokenizer");
        goto end;
     }

   stream = fopen(filename, "rb");
   if (!stream)
     {
        ERR("unable to read in %s", filename);
        goto end;
     }

   buffer = malloc(BUFSIZE);
   if (!buffer)
     {
        ERR("unable to allocate read buffer");
        goto end;
     }

   unsigned int len = fread(buffer, 1, BUFSIZE, stream);

   if (!len)
     {
        ERR("%s: is an empty file", filename);
        goto end;
     }

   if (len == BUFSIZE)
     {
        ERR("%s: buffer(%d) is full, might not be big enough.", filename, len);
        goto end;
     }

   buffer[len] = '\0';
#if _WIN32
   {
      Eina_Strbuf *str_buffer = eina_strbuf_manage_new(buffer);
      if (eina_strbuf_replace_all(str_buffer, "\r\n", "\n"))
         len = eina_strbuf_length_get(str_buffer);
      buffer = eina_strbuf_string_steal(str_buffer);
      eina_strbuf_free(str_buffer);
   }
#endif
   if (!eo_tokenizer_mem_walk(toknz, filename, buffer, len)) goto end;

   if (!toknz->classes)
     {
        ERR("No classes for file %s", filename);
        goto end;
     }

   EINA_LIST_FOREACH(toknz->classes, k, kls)
     {
        Eolian_Class class = database_class_add(kls->name, kls->type);
        database_class_file_set(class, filename);

        if (kls->comment) database_class_description_set(class, kls->comment);

        EINA_LIST_FOREACH(kls->inherits, l, s)
           database_class_inherit_add(class, s);

        if (kls->legacy_prefix)
          {
             database_class_legacy_prefix_set(class, kls->legacy_prefix);
          }
        if (kls->eo_prefix)
          {
             database_class_eo_prefix_set(class, kls->eo_prefix);
          }
        if (kls->data_type)
          {
             database_class_data_type_set(class, kls->data_type);
          }
        EINA_LIST_FOREACH(kls->constructors, l, meth)
          {
             Eolian_Function foo_id = database_function_new(meth->name, EOLIAN_CTOR);
             database_class_function_add(class, foo_id);
             if (meth->ret) database_function_return_comment_set(foo_id, EOLIAN_METHOD, meth->ret->comment);
             database_function_data_set(foo_id, EOLIAN_LEGACY, meth->legacy);
             EINA_LIST_FOREACH(meth->params, m, param)
               {
                  Eolian_Type type = _types_extract(param->type, strlen(param->type));
                  database_method_parameter_add(foo_id, (Eolian_Parameter_Dir)param->way, type, param->name, param->comment);
               }
          }

        EINA_LIST_FOREACH(kls->properties, l, prop)
          {
             Eolian_Function foo_id = database_function_new(prop->name, EOLIAN_UNRESOLVED);
             database_function_scope_set(foo_id, prop->scope);
             database_function_set_as_ctor(foo_id, prop->is_ctor);
             EINA_LIST_FOREACH(prop->keys, m, param)
               {
                  Eolian_Type type = _types_extract(param->type, strlen(param->type));
                  Eolian_Function_Parameter p = database_property_key_add(
                        foo_id, type, param->name, param->comment);
                  database_parameter_nonull_set(p, param->nonull);
               }
             EINA_LIST_FOREACH(prop->values, m, param)
               {
                  Eolian_Type type = _types_extract(param->type, strlen(param->type));
                  Eolian_Function_Parameter p = database_property_value_add(
                        foo_id, type, param->name, param->comment);
                  database_parameter_nonull_set(p, param->nonull);
               }
             EINA_LIST_FOREACH(prop->accessors, m, accessor)
               {
                  database_function_type_set(foo_id, (accessor->type == SETTER?EOLIAN_PROP_SET:EOLIAN_PROP_GET));
                  if (accessor->ret && accessor->ret->type)
                    {
                       Eolian_Function_Type ftype =
                          accessor->type == SETTER?EOLIAN_PROP_SET:EOLIAN_PROP_GET;
                       Eolian_Type types = _types_extract(accessor->ret->type, strlen(accessor->ret->type));
                       database_function_return_type_set(foo_id, ftype, types);
                       database_function_return_comment_set(foo_id,
                             ftype, accessor->ret->comment);
                       database_function_return_flag_set_as_warn_unused(foo_id,
                             ftype, accessor->ret->warn_unused);
                       database_function_return_dflt_val_set(foo_id,
                             ftype, accessor->ret->dflt_ret_val);
                    }
                  if (accessor->legacy)
                    {
                       database_function_data_set(foo_id,
                             (accessor->type == SETTER?EOLIAN_LEGACY_SET:EOLIAN_LEGACY_GET),
                             accessor->legacy);
                    }
                  database_function_description_set(foo_id,
                        (accessor->type == SETTER?EOLIAN_COMMENT_SET:EOLIAN_COMMENT_GET),
                        accessor->comment);
                  Eo_Accessor_Param *acc_param;
                  Eina_List *m2;
                  EINA_LIST_FOREACH(accessor->params, m2, acc_param)
                    {
                       Eolian_Function_Parameter desc = eolian_function_parameter_get(foo_id, acc_param->name);
                       if (!desc)
                         {
                            printf("Error - %s not known as parameter of property %s\n", acc_param->name, prop->name);
                         }
                       else
                          if (strstr(acc_param->attrs, "const"))
                            {
                               database_parameter_const_attribute_set(desc, accessor->type == GETTER, EINA_TRUE);
                            }
                    }
               }
             if (!prop->accessors) database_function_type_set(foo_id, EOLIAN_PROPERTY);
             database_class_function_add(class, foo_id);
          }

        EINA_LIST_FOREACH(kls->methods, l, meth)
          {
             Eolian_Function foo_id = database_function_new(meth->name, EOLIAN_METHOD);
             database_function_scope_set(foo_id, meth->scope);
             database_class_function_add(class, foo_id);
             if (meth->ret)
               {
                  Eolian_Type types = _types_extract(meth->ret->type, strlen(meth->ret->type));
                  database_function_return_type_set(foo_id, EOLIAN_METHOD, types);
                  database_function_return_comment_set(foo_id, EOLIAN_METHOD, meth->ret->comment);
                  database_function_return_flag_set_as_warn_unused(foo_id,
                        EOLIAN_METHOD, meth->ret->warn_unused);
                  database_function_return_dflt_val_set(foo_id,
                        EOLIAN_METHOD, meth->ret->dflt_ret_val);
               }
             database_function_description_set(foo_id, EOLIAN_COMMENT, meth->comment);
             database_function_data_set(foo_id, EOLIAN_LEGACY, meth->legacy);
             database_function_object_set_as_const(foo_id, meth->obj_const);
             database_function_set_as_ctor(foo_id, meth->is_ctor);
             EINA_LIST_FOREACH(meth->params, m, param)
               {
                  Eolian_Type type = _types_extract(param->type, strlen(param->type));
                  Eolian_Function_Parameter p = database_method_parameter_add(foo_id,
                        (Eolian_Parameter_Dir)param->way, type, param->name, param->comment);
                  database_parameter_nonull_set(p, param->nonull);
               }
          }

        EINA_LIST_FOREACH(kls->implements, l, impl)
          {
             const char *impl_name = impl->meth_name;
             if (!strcmp(impl_name, "class::constructor"))
               {
                  database_class_ctor_enable_set(class, EINA_TRUE);
                  continue;
               }
             if (!strcmp(impl_name, "class::destructor"))
               {
                  database_class_dtor_enable_set(class, EINA_TRUE);
                  continue;
               }
             if (!strncmp(impl_name, "virtual::", 9))
               {
                  char *virtual_name = strdup(impl_name);
                  char *func = strstr(virtual_name, "::");
                  if (func) *func = '\0';
                  func += 2;
                  Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
                  char *type_as_str = strstr(func, "::");
                  if (type_as_str)
                    {
                       *type_as_str = '\0';
                       if (!strcmp(type_as_str+2, "set")) ftype = EOLIAN_PROP_SET;
                       else if (!strcmp(type_as_str+2, "get")) ftype = EOLIAN_PROP_GET;
                    }
                  /* Search the function into the existing functions of the current class */
                  Eolian_Function foo_id = eolian_class_function_find_by_name(
                        class, func, ftype);
                  free(virtual_name);
                  if (!foo_id)
                    {
                       ERR("Error - %s not known in class %s", impl_name + 9, eolian_class_name_get(class));
                       goto end;
                    }
                  database_function_set_as_virtual_pure(foo_id, ftype);
                  continue;
               }
             Eolian_Implement impl_desc = database_implement_new(impl_name);
             database_class_implement_add(class, impl_desc);
          }

        EINA_LIST_FOREACH(kls->events, l, event)
          {
             Eolian_Event ev = database_event_new(event->name, event->type, event->comment);
             database_class_event_add(class, ev);
          }

     }

   EINA_LIST_FOREACH(toknz->typedefs, k, type_def)
     {
        database_type_add(type_def->alias, _types_extract(type_def->type, strlen(type_def->type)));
     }

   ret = EINA_TRUE;
end:
   if (buffer) free(buffer);
   if (stream) fclose(stream);
   if (toknz) eo_tokenizer_free(toknz);
   return ret;
}

void
eo_tokenizer_free(Eo_Tokenizer *toknz)
{
   Eo_Class_Def *kls;
   Eo_Type_Def *type;

   if (toknz->source)
     eina_stringshare_del(toknz->source);

   EINA_LIST_FREE(toknz->classes, kls)
      eo_definitions_class_def_free(kls);

   EINA_LIST_FREE(toknz->typedefs, type)
      eo_definitions_type_def_free(type);

   free(toknz);
}

