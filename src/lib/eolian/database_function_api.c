#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

EAPI Eolian_Object_Scope
eolian_function_scope_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EOLIAN_SCOPE_PUBLIC);
   return fid->scope;
}

EAPI Eolian_Function_Type
eolian_function_type_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EOLIAN_UNRESOLVED);
   return fid->type;
}

EAPI Eina_Stringshare *
eolian_function_name_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   return fid->name;
}

static const char *
get_eo_prefix(const Eolian_Function *foo_id, char *buf)
{
    char *tmp = buf;
    if (foo_id->klass->eo_prefix)
      return foo_id->klass->eo_prefix;
    strcpy(buf, foo_id->klass->full_name);
    eina_str_tolower(&buf);
    while ((tmp = strchr(tmp, '.'))) *tmp = '_';
    return buf;
}

EAPI Eina_Stringshare *
eolian_function_full_c_name_get(const Eolian_Function *foo_id)
{
   char tbuf[512];
   const char  *prefix = get_eo_prefix(foo_id, tbuf);
   const char  *funcn = eolian_function_name_get(foo_id);
   const char  *last_p = strrchr(prefix, '_');
   const char  *func_p = strchr(funcn, '_');
   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Stringshare *ret;
   int   len;

   if (!last_p) last_p = prefix;
   else last_p++;
   if (!func_p) len = strlen(funcn);
   else len = func_p - funcn;

   if ((int)strlen(last_p) != len || strncmp(last_p, funcn, len))
     {
        eina_strbuf_append(buf, prefix);
        eina_strbuf_append_char(buf, '_');
        eina_strbuf_append(buf, funcn);
        ret = eina_stringshare_add(eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
        return ret;
     }

   if (last_p != prefix)
      eina_strbuf_append_n(buf, prefix, last_p - prefix); /* includes _ */

   eina_strbuf_append(buf, funcn);
   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

EAPI Eina_Stringshare *
eolian_function_legacy_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   switch (ftype)
     {
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_legacy; break;
      case EOLIAN_PROP_SET: return fid->set_legacy; break;
      default: return NULL;
     }
}

EAPI Eina_Stringshare *
eolian_function_description_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   switch (ftype)
     {
      case EOLIAN_PROP_GET: return fid->get_description; break;
      case EOLIAN_PROP_SET: return fid->set_description; break;
      default: return fid->common_description;
     }
}

EAPI Eina_Bool
eolian_function_is_virtual_pure(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_virtual_pure; break;
      case EOLIAN_PROP_SET: return fid->set_virtual_pure; break;
      default: return EINA_FALSE;
     }
}

EAPI Eina_Bool
eolian_function_is_auto(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_auto; break;
      case EOLIAN_PROP_SET: return fid->set_auto; break;
      default: return EINA_FALSE;
     }
}

EAPI Eina_Bool
eolian_function_is_empty(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_empty; break;
      case EOLIAN_PROP_SET: return fid->set_empty; break;
      default: return EINA_FALSE;
     }
}

EAPI Eina_Bool
eolian_function_is_legacy_only(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_only_legacy; break;
      case EOLIAN_PROP_SET: return fid->set_only_legacy; break;
      default: return EINA_FALSE;
     }
}

EAPI Eina_Bool
eolian_function_is_class(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   return fid->is_class;
}

EAPI Eina_Bool
eolian_function_is_constructor(const Eolian_Function *fid, const Eolian_Class *klass)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   Eina_Stringshare *s = eina_stringshare_ref(klass->full_name);
   Eina_Bool r = !!eina_list_search_sorted_list
     (fid->ctor_of, EINA_COMPARE_CB(strcmp), s);
   eina_stringshare_del(s);
   return r;
}

EAPI const Eolian_Function_Parameter *
eolian_function_parameter_get_by_name(const Eolian_Function *fid, const char *param_name)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   Eina_List *itr;
   Eolian_Function_Parameter *param;
   EINA_LIST_FOREACH(fid->keys, itr, param)
      if (!strcmp(param->name, param_name)) return param;
   EINA_LIST_FOREACH(fid->params, itr, param)
      if (!strcmp(param->name, param_name)) return param;
   return NULL;
}

EAPI Eina_Iterator *
eolian_property_keys_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   return (fid->keys ? eina_list_iterator_new(fid->keys) : NULL);
}

EAPI Eina_Iterator *
eolian_property_values_get(const Eolian_Function *fid)
{
   return eolian_function_parameters_get(fid);
}

EAPI Eina_Iterator *
eolian_function_parameters_get(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, NULL);
   return (fid->params ? eina_list_iterator_new(fid->params) : NULL);
}

EAPI const Eolian_Type *
eolian_function_return_type_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   switch (ftype)
     {
      case EOLIAN_PROP_SET: return fid->set_ret_type;
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROP_GET: return fid->get_ret_type;
      default: return NULL;
     }
}

EAPI const Eolian_Expression *
eolian_function_return_default_value_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   switch (ftype)
     {
      case EOLIAN_PROP_SET: return fid->set_ret_val;
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_ret_val;
      default: return NULL;
     }
}

EAPI Eina_Stringshare *
eolian_function_return_comment_get(const Eolian_Function *fid, Eolian_Function_Type ftype)
{
   switch (ftype)
     {
      case EOLIAN_PROP_SET: return fid->set_return_comment; break;
      case EOLIAN_UNRESOLVED: case EOLIAN_METHOD: case EOLIAN_PROPERTY: case EOLIAN_PROP_GET: return fid->get_return_comment; break;
      default: return NULL;
     }
}

EAPI Eina_Bool
eolian_function_return_is_warn_unused(const Eolian_Function *fid,
      Eolian_Function_Type ftype)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   switch (ftype)
     {
      case EOLIAN_METHOD: case EOLIAN_PROP_GET: case EOLIAN_PROPERTY: return fid->get_return_warn_unused;
      case EOLIAN_PROP_SET: return fid->set_return_warn_unused;
      default: return EINA_FALSE;
     }
}

EAPI Eina_Bool
eolian_function_object_is_const(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   return fid->obj_is_const;
}

EAPI Eina_Bool
eolian_function_is_c_only(const Eolian_Function *fid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fid, EINA_FALSE);
   return fid->is_c_only;
}
