#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include<Eina.h>
#include "Eolian.h"
#include "eolian_suite.h"

START_TEST(eolian_namespaces)
{
   const Eolian_Class *class11, *class112, *class21, *class_no, *impl_class;
   const Eolian_Function *fid;
   Eina_Iterator *iter;
   Eolian_Function_Type func_type;
   const char *class_name, *val1, *val2;
   const Eolian_Implement *impl;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/namespace.eo"));

   /* Classes existence  */
   fail_if(!(class11 = eolian_class_find_by_name("nmsp1.class1")));
   fail_if(!(class112 = eolian_class_find_by_name("nmsp1.nmsp11.class2")));
   fail_if(!(class21 = eolian_class_find_by_name("nmsp2.class1")));
   fail_if(!(class_no = eolian_class_find_by_name("no_nmsp")));

   /* Check names and namespaces*/
   fail_if(strcmp(eolian_class_name_get(class11), "class1"));
   fail_if(!(iter = eolian_class_namespaces_list_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp1"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class112), "class2"));
   fail_if(!(iter = eolian_class_namespaces_list_get(class112)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(!(eina_iterator_next(iter, (void**)&val2)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp1"));
   fail_if(strcmp(val2, "nmsp11"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class21), "class1"));
   fail_if(!(iter = eolian_class_namespaces_list_get(class21)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp2"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class_no), "no_nmsp"));
   fail_if(eolian_class_namespaces_list_get(class_no));

   /* Inherits */
   fail_if(!(iter = eolian_class_inherits_list_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_find_by_name(class_name) != class112);
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_find_by_name(class_name) != class21);
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_find_by_name(class_name) != class_no);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* Implements */
   fail_if(!(iter = eolian_class_implements_list_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_information_get(impl,
            &impl_class, &fid, &func_type));
   fail_if(impl_class != class112);
   fail_if(strcmp(eolian_function_name_get(fid), "a"));
   fail_if(func_type != EOLIAN_PROP_SET);

   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(eolian_implement_information_get(impl,
            &impl_class, &fid, &func_type));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_information_get(impl,
            &impl_class, &fid, &func_type));
   fail_if(impl_class != class_no);
   fail_if(strcmp(eolian_function_name_get(fid), "foo"));
   fail_if(func_type != EOLIAN_METHOD);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* Virtual regression */
   fail_if(!(fid = eolian_class_function_find_by_name(class112, "a", EOLIAN_UNRESOLVED)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_PROP_SET));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_events)
{
   const Eolian_Class *class;
   Eina_Iterator *iter;
   const char *name, *comment, *type_name;
   const Eolian_Type *type;
   const Eolian_Event *ev;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/events.eo"));

   /* Class */
   fail_if(!(class = eolian_class_find_by_name("Events")));

   /* Events */
   fail_if(!(iter = eolian_class_events_list_get(class)));
   /* Clicked */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!eolian_class_event_information_get(ev, &name, &type, &comment));
   fail_if(strcmp(name, "clicked"));
   fail_if(type);
   fail_if(strcmp(comment, "Comment for clicked"));
   /* Clicked,double */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!eolian_class_event_information_get(ev, &name, &type, &comment));
   fail_if(strcmp(name, "clicked,double"));
   fail_if(!type);
   type_name = eolian_type_name_get(type);
   fail_if(strcmp(type_name, "Evas_Event_Clicked_Double_Info"));
   eina_stringshare_del(type_name);
   fail_if(comment);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_override)
{
   Eina_Iterator *iter;
   const Eolian_Function *fid = NULL;
   const Eolian_Class *impl_class = NULL;
   const Eolian_Function *impl_func = NULL;
   const Eolian_Class *class, *base;
   const Eolian_Implement *impl;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/override.eo"));

   /* Class */
   fail_if(!(class = eolian_class_find_by_name("Simple")));
   fail_if(!(base = eolian_class_find_by_name("Base")));

   /* Base ctor */
   fail_if(!(fid = eolian_class_function_find_by_name(base, "constructor", EOLIAN_UNRESOLVED)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_UNRESOLVED));
   fail_if(!(iter = eolian_class_implements_list_get(class)));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_information_get(impl, &impl_class, &impl_func, NULL));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "constructor"));
   eina_iterator_free(iter);

   /* Property */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_PROP_SET));
   fail_if(eolian_function_is_virtual_pure(fid, EOLIAN_PROP_GET));

   /* Method */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_METHOD));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_consts)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Function_Parameter *param = NULL;
   const Eolian_Class *class;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/consts.eo"));
   fail_if(!(class = eolian_class_find_by_name("Const")));

   /* Property */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!(param = eolian_function_parameter_get(fid, "buffer")));
   fail_if(eolian_parameter_const_attribute_get(param, EINA_FALSE));
   fail_if(!eolian_parameter_const_attribute_get(param, EINA_TRUE));

   /* Method */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(EINA_FALSE == eolian_function_object_is_const(fid));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_ctor_dtor)
{
   Eina_Iterator *iter;
   const Eolian_Class *impl_class = NULL;
   const Eolian_Function *impl_func = NULL;
   const Eolian_Class *class, *base;
   const Eolian_Implement *impl;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/ctor_dtor.eo"));
   fail_if(!(class = eolian_class_find_by_name("Ctor_Dtor")));
   fail_if(!(base = eolian_class_find_by_name("Base")));

   /* Class ctor/dtor */
   fail_if(!eolian_class_ctor_enable_get(class));
   fail_if(!eolian_class_dtor_enable_get(class));

   /* Base ctor/dtor */
   fail_if(!(iter = eolian_class_implements_list_get(class)));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_information_get(impl, &impl_class, &impl_func, NULL));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "constructor"));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_information_get(impl, &impl_class, &impl_func, NULL));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "destructor"));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* Custom ctors/dtors */
   fail_if(!eolian_class_function_find_by_name(base, "constructor", EOLIAN_CTOR));
   fail_if(!eolian_class_function_find_by_name(base, "destructor", EOLIAN_METHOD));
   fail_if(!eolian_class_function_find_by_name(class, "custom_constructor_1", EOLIAN_CTOR));
   fail_if(!eolian_class_function_find_by_name(class, "custom_constructor_2", EOLIAN_CTOR));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_typedef)
{
   const Eolian_Type *atype = NULL, *type = NULL;
   const char *type_name = NULL;
   Eina_Iterator *iter = NULL;
   const Eolian_Class *class;
   const char *file;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/typedef.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_find_by_name("Dummy")));
   fail_if(!eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD));

   /* Basic type */
   fail_if(!(atype = eolian_type_alias_find_by_name("Evas.Coord")));
   fail_if(eolian_type_type_get(atype) != EOLIAN_TYPE_ALIAS);
   fail_if(!(type_name = eolian_type_name_get(atype)));
   fail_if(strcmp(type_name, "Coord"));
   eina_stringshare_del(type_name);
   fail_if(!(type_name = eolian_type_c_type_get(atype)));
   fail_if(strcmp(type_name, "typedef int Evas_Coord"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(atype)));
   fail_if(!(type_name = eolian_type_name_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(eolian_type_is_const(type));
   fail_if(eolian_type_base_type_get(type));
   fail_if(strcmp(type_name, "int"));
   eina_stringshare_del(type_name);

   /* File */
   fail_if(!(file = eolian_type_file_get(atype)));
   fail_if(strcmp(file, "typedef.eo"));
   eina_stringshare_del(file);

   /* Complex type */
   fail_if(!(atype = eolian_type_alias_find_by_name("List_Objects")));
   fail_if(!(type_name = eolian_type_name_get(atype)));
   fail_if(strcmp(type_name, "List_Objects"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(atype)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(iter = eolian_type_subtypes_list_get(type)));
   fail_if(!eina_iterator_next(iter, (void**)&type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(strcmp(type_name, "Eo *"));
   fail_if(eolian_type_is_own(type));
   eina_stringshare_del(type_name);
   eina_iterator_free(iter);

   /* List */
   fail_if(!(iter = eolian_type_aliases_get_by_file("typedef.eo")));
   fail_if(!eina_iterator_next(iter, (void**)&atype));
   fail_if(!(type_name = eolian_type_name_get(atype)));
   fail_if(strcmp(type_name, "Coord"));
   eina_stringshare_del(type_name);
   fail_if(!eina_iterator_next(iter, (void**)&atype));
   fail_if(!(type_name = eolian_type_name_get(atype)));
   fail_if(strcmp(type_name, "List_Objects"));
   eina_stringshare_del(type_name);
   fail_if(eina_iterator_next(iter, (void**)&atype));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_complex_type)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Function_Parameter *param = NULL;
   const Eolian_Type *type = NULL;
   const char *type_name = NULL;
   Eina_Iterator *iter = NULL;
   const Eolian_Class *class;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/complex_type.eo"));
   fail_if(!(class = eolian_class_find_by_name("Complex_Type")));

   /* Properties return type */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!(type = eolian_function_return_type_get(fid, EOLIAN_PROP_SET)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(iter = eolian_type_subtypes_list_get(type)));
   fail_if(!eina_iterator_next(iter, (void**)&type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_Array *"));
   eina_stringshare_del(type_name);
   eina_iterator_free(iter);
   fail_if(!(iter = eolian_type_subtypes_list_get(type)));
   fail_if(!eina_iterator_next(iter, (void**)&type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eo **"));
   eina_stringshare_del(type_name);
   eina_iterator_free(iter);
   /* Properties parameter type */
   fail_if(!(iter = eolian_parameters_list_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   fail_if(strcmp(eolian_parameter_name_get(param), "value"));
   fail_if(!(type = eolian_parameter_type_get(param)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(iter = eolian_type_subtypes_list_get(type)));
   fail_if(!eina_iterator_next(iter, (void**)&type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "int"));
   eina_stringshare_del(type_name);
   eina_iterator_free(iter);

   /* Methods return type */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!(type = eolian_function_return_type_get(fid, EOLIAN_METHOD)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(iter = eolian_type_subtypes_list_get(type)));
   fail_if(!eina_iterator_next(iter, (void**)&type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_Stringshare *"));
   eina_stringshare_del(type_name);
   eina_iterator_free(iter);
   /* Methods parameter type */
   fail_if(!(iter = eolian_parameters_list_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   fail_if(strcmp(eolian_parameter_name_get(param), "buf"));
   fail_if(!(type = eolian_parameter_type_get(param)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "char *"));
   eina_stringshare_del(type_name);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_scope)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Class *class;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/scope.eo"));
   fail_if(!(class = eolian_class_find_by_name("Simple")));

   /* Property scope */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PROTECTED);
   fail_if(!(fid = eolian_class_function_find_by_name(class, "b", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_find_by_name(class, "c", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PUBLIC);

   /* Method scope */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_find_by_name(class, "bar", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PROTECTED);
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foobar", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid) != EOLIAN_SCOPE_PUBLIC);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_simple_parsing)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Type *ptypep = NULL;
   const char *string = NULL, *ptype = NULL, *pname = NULL;
   Eolian_Parameter_Dir dir = EOLIAN_IN_PARAM;
   const Eolian_Function_Parameter *param = NULL;
   const Eolian_Class *class;
   const Eolian_Type *tp;
   Eina_Iterator *iter;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/class_simple.eo"));
   fail_if(!(class = eolian_class_find_by_name("Simple")));
   fail_if(eolian_class_find_by_file(PACKAGE_DATA_DIR"/data/class_simple.eo") != class);
   fail_if(strcmp(eolian_class_file_get(class), PACKAGE_DATA_DIR"/data/class_simple.eo"));

   /* Class */
   fail_if(eolian_class_type_get(class) != EOLIAN_CLASS_REGULAR);
   string = eolian_class_description_get(class);
   fail_if(!string);
   fail_if(strcmp(string, "Class Desc Simple"));
   fail_if(eolian_class_inherits_list_get(class) != NULL);
   fail_if(strcmp(eolian_class_legacy_prefix_get(class), "evas_object_simple"));
   fail_if(strcmp(eolian_class_eo_prefix_get(class), "evas_obj_simple"));
   fail_if(strcmp(eolian_class_data_type_get(class), "Evas_Simple_Data"));

   /* Property */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(strcmp(eolian_function_name_get(fid), "a"));
   string = eolian_function_description_get(fid, EOLIAN_COMMENT_SET);
   fail_if(!string);
   fail_if(strcmp(string, "comment a.set"));
   string = eolian_function_description_get(fid, EOLIAN_COMMENT_GET);
   fail_if(string);
   /* Set return */
   tp = eolian_function_return_type_get(fid, EOLIAN_PROP_SET);
   fail_if(!tp);
   fail_if(strcmp(eolian_type_name_get(tp), "Eina_Bool"));
   string = eolian_function_return_default_value_get(fid, EOLIAN_PROP_SET);
   fail_if(!string);
   fail_if(strcmp(string, "EINA_TRUE"));
   string = eolian_function_return_comment_get(fid, EOLIAN_PROP_SET);
   fail_if(!string);
   fail_if(strcmp(string, "comment for property set return"));
   /* Get return */
   tp = eolian_function_return_type_get(fid, EOLIAN_PROP_GET);
   fail_if(tp);
   string = eolian_function_return_comment_get(fid, EOLIAN_PROP_GET);
   fail_if(string);

   /* Function parameters */
   fail_if(eolian_property_keys_list_get(fid) != NULL);
   fail_if(!(iter = eolian_property_values_list_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   eolian_parameter_information_get(param, NULL, &ptypep, &pname, &string);
   fail_if(strcmp(eolian_type_name_get(ptypep), "int"));
   fail_if(strcmp(pname, "value"));
   fail_if(strcmp(string, "Value description"));

   /* Method */
   fail_if(!(fid = eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD)));
   string = eolian_function_description_get(fid, EOLIAN_COMMENT);
   fail_if(!string);
   fail_if(strcmp(string, "comment foo"));
   /* Function return */
   tp = eolian_function_return_type_get(fid, EOLIAN_METHOD);
   fail_if(!tp);
   string = eolian_type_c_type_get(tp);
   fail_if(!string);
   fail_if(strcmp(string, "char *"));
   eina_stringshare_del(string);
   string = eolian_function_return_default_value_get(fid, EOLIAN_METHOD);
   fail_if(!string);
   fail_if(strcmp(string, "NULL"));
   string = eolian_function_return_comment_get(fid, EOLIAN_METHOD);
   fail_if(!string);
   fail_if(strcmp(string, "comment for method return"));

   /* Function parameters */
   fail_if(!(iter = eolian_property_values_list_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   eolian_parameter_information_get(param, &dir, &ptypep, &pname, &string);
   fail_if(dir != EOLIAN_IN_PARAM);
   fail_if(strcmp(eolian_type_name_get(ptypep), "int"));
   fail_if(strcmp(pname, "a"));
   fail_if(!string || strcmp(string, "a"));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   eolian_parameter_information_get(param, &dir, &ptypep, &pname, &string);
   fail_if(dir != EOLIAN_INOUT_PARAM);
   ptype = eolian_type_name_get(ptypep);
   fail_if(strcmp(ptype, "char"));
   eina_stringshare_del(ptype);
   fail_if(strcmp(pname, "b"));
   fail_if(string);
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   eolian_parameter_information_get(param, &dir, &ptypep, &pname, &string);
   fail_if(dir != EOLIAN_OUT_PARAM);
   fail_if(strcmp(eolian_type_name_get(ptypep), "double"));
   fail_if(strcmp(pname, "c"));
   fail_if(string);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_struct)
{
   const Eolian_Type *atype = NULL, *type = NULL, *field = NULL;
   const Eolian_Class *class;
   const char *type_name;
   const char *file;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/struct.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_find_by_name("Dummy")));
   fail_if(!eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD));

   /* named struct */
   fail_if(!(type = eolian_type_struct_find_by_name("Named")));
   fail_if(!(type_name = eolian_type_name_get(type)));
   fail_if(!(file = eolian_type_file_get(type)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_STRUCT);
   fail_if(eolian_type_is_own(type));
   fail_if(eolian_type_is_const(type));
   fail_if(strcmp(type_name, "Named"));
   eina_stringshare_del(type_name);
   fail_if(strcmp(file, "struct.eo"));
   eina_stringshare_del(file);
   fail_if(!(field = eolian_type_struct_field_get(type, "field")));
   fail_if(!(type_name = eolian_type_name_get(field)));
   fail_if(strcmp(type_name, "int"));
   eina_stringshare_del(type_name);
   fail_if(!(field = eolian_type_struct_field_get(type, "something")));
   fail_if(!(type_name = eolian_type_c_type_get(field)));
   fail_if(strcmp(type_name, "const char *"));
   eina_stringshare_del(type_name);

   /* referencing */
   fail_if(!(type = eolian_type_struct_find_by_name("Another")));
   fail_if(!(type_name = eolian_type_name_get(type)));
   fail_if(!(file = eolian_type_file_get(type)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_STRUCT);
   fail_if(strcmp(type_name, "Another"));
   eina_stringshare_del(type_name);
   fail_if(strcmp(file, "struct.eo"));
   eina_stringshare_del(file);
   fail_if(!(field = eolian_type_struct_field_get(type, "field")));
   fail_if(!(type_name = eolian_type_name_get(field)));
   fail_if(strcmp(type_name, "Named"));
   eina_stringshare_del(type_name);
   fail_if(eolian_type_type_get(field) != EOLIAN_TYPE_REGULAR_STRUCT);

   /* typedef */
   fail_if(!(atype = eolian_type_alias_find_by_name("Foo")));
   fail_if(!(type = eolian_type_base_type_get(atype)));
   fail_if(!(type_name = eolian_type_name_get(type)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_STRUCT);
   fail_if(strcmp(type_name, "_Foo"));
   eina_stringshare_del(type_name);

   /* typedef - anon */
   fail_if(!(atype = eolian_type_alias_find_by_name("Bar")));
   fail_if(!(type = eolian_type_base_type_get(atype)));
   fail_if(!!(type_name = eolian_type_name_get(type)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_STRUCT);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_extern)
{
   const Eolian_Type *type = NULL;
   const Eolian_Class *class;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_eo_file_parse(PACKAGE_DATA_DIR"/data/extern.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_find_by_name("Dummy")));
   fail_if(!eolian_class_function_find_by_name(class, "foo", EOLIAN_METHOD));

   /* regular type */
   fail_if(!(type = eolian_type_alias_find_by_name("Foo")));
   fail_if(eolian_type_is_extern(type));

   /* extern type */
   fail_if(!(type = eolian_type_alias_find_by_name("Evas.Coord")));
   fail_if(!eolian_type_is_extern(type));

   /* regular struct */
   fail_if(!(type = eolian_type_struct_find_by_name("X")));
   fail_if(eolian_type_is_extern(type));

   /* extern struct */
   fail_if(!(type = eolian_type_struct_find_by_name("Y")));
   fail_if(!eolian_type_is_extern(type));

   eolian_shutdown();
}
END_TEST

void eolian_parsing_test(TCase *tc)
{
   tcase_add_test(tc, eolian_simple_parsing);
   tcase_add_test(tc, eolian_ctor_dtor);
   tcase_add_test(tc, eolian_scope);
   tcase_add_test(tc, eolian_complex_type);
   tcase_add_test(tc, eolian_typedef);
   tcase_add_test(tc, eolian_consts);
   tcase_add_test(tc, eolian_override);
   tcase_add_test(tc, eolian_events);
   tcase_add_test(tc, eolian_namespaces);
   tcase_add_test(tc, eolian_struct);
   tcase_add_test(tc, eolian_extern);
}

