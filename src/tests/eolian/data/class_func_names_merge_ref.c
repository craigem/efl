
void _efl_text_text_set(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_set);

void _efl_text_text_get(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_get);

void _efl_text_text2_set(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_text2_set);

void _efl_text_text2_get(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_text2_get);

void _efl_text_text_size_set(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_size_set);

void _efl_text_text_size_get(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_size_get);

void _efl_text_text_markup_set(Eo *obj, Efl_Text_Data *pd);

EOAPI EO_VOID_FUNC_BODY(efl_text_markup_set);

static Eo_Op_Description _efl_text_op_desc[] = {
     EO_OP_FUNC(efl_text_set, _efl_text_text_set, ""),
     EO_OP_FUNC(efl_text_get, _efl_text_text_get, ""),
     EO_OP_FUNC(efl_text_text2_set, _efl_text_text2_set, ""),
     EO_OP_FUNC(efl_text_text2_get, _efl_text_text2_get, ""),
     EO_OP_FUNC(efl_text_size_set, _efl_text_text_size_set, ""),
     EO_OP_FUNC(efl_text_size_get, _efl_text_text_size_get, ""),
     EO_OP_FUNC(efl_text_markup_set, _efl_text_text_markup_set, ""),
     EO_OP_SENTINEL
};

static const Eo_Class_Description _efl_text_class_desc = {
     EO_VERSION,
     "Efl_Text",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(_efl_text_op_desc),
     NULL,
     sizeof(Efl_Text_Data),
     NULL,
     NULL
};

EO_DEFINE_CLASS(efl_text_class_get, &_efl_text_class_desc, NULL, NULL);
EAPI void
efl_text_set(Eo *obj)
{
   eo_do((Eo *) obj, efl_text_set());
}

EAPI void
efl_text_get(const Eo *obj)
{
   eo_do((Eo *) obj, efl_text_get());
}

EAPI void
efl_text_text2_set(Eo *obj)
{
   eo_do((Eo *) obj, efl_text_text2_set());
}

EAPI void
efl_text_text2_get(const Eo *obj)
{
   eo_do((Eo *) obj, efl_text_text2_get());
}

EAPI void
efl_text_size_set(Eo *obj)
{
   eo_do((Eo *) obj, efl_text_size_set());
}

EAPI void
efl_text_size_get(const Eo *obj)
{
   eo_do((Eo *) obj, efl_text_size_get());
}

EAPI void
efl_text_markup_set(Eo *obj)
{
   eo_do((Eo *) obj, efl_text_markup_set());
}
