#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <string.h>

#include "Eolian.h"

#include "legacy_generator.h"
#include "common_funcs.h"

static _eolian_class_vars class_env;

static const char
tmpl_eapi_funcdef[] = "\n\
/**\n\
 *\n\
@#desc\n\
 *\n\
@#list_desc_param\
 */\n\
EAPI @#type_return%s(@#params)@#flags;\n\
";

/*@#CLASS_CHECK(obj) @#check_ret;\n\*/
static const char
tmpl_eapi_body[] ="\
\n\
EAPI @#ret_type\n\
@#eapi_func(@#full_params)\n\
{\n\
   return eo_do(@#eo_obj, @#eo_func(@#eo_params));\n\
}\n\
";
static const char
tmpl_eapi_body_void[] ="\
\n\
EAPI void\n\
@#eapi_func(@#full_params)\n\
{\n\
   eo_do(@#eo_obj, @#eo_func(@#eo_params));\n\
}\n\
";

static void
_eapi_decl_func_generate(const Eolian_Class *class, const Eolian_Function *funcid, Eolian_Function_Type ftype, Eina_Strbuf *buf)
{
   _eolian_class_func_vars func_env;
   const char *funcname = eolian_function_name_get(funcid);
   const Eolian_Type *rettypet = NULL;
   const char *rettype = NULL;
   Eina_Bool var_as_ret = EINA_FALSE;
   Eina_Bool add_star = EINA_FALSE;
   Eina_Bool ret_const = EINA_FALSE;
   Eina_Iterator *itr;
   void *data, *data2;
   Eina_Strbuf *flags = NULL;
   int leg_param_idx = 1; /* Index of the parameter inside the legacy function. It begins from 1 since obj is the first. */

   Eina_Strbuf *fbody = eina_strbuf_new();
   Eina_Strbuf *fparam = eina_strbuf_new();
   Eina_Strbuf *descparam = eina_strbuf_new();

   _class_func_env_create(class, funcname, ftype, &func_env);
   rettypet = eolian_function_return_type_get(funcid, ftype);
   if (ftype == EOLIAN_PROP_GET)
     {
        add_star = EINA_TRUE;
        if (!rettypet)
          {
             itr = eolian_function_parameters_get(funcid);
             /* We want to check if there is only one parameter */
             if (eina_iterator_next(itr, &data) && !eina_iterator_next(itr, &data2))
               {
                  rettypet = eolian_parameter_type_get((Eolian_Function_Parameter*)data);
                  var_as_ret = EINA_TRUE;
                  ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
               }
             eina_iterator_free(itr);
          }
     }

   if (func_env.legacy_func[0] == '\0') goto end;
   eina_strbuf_append_printf(fbody, tmpl_eapi_funcdef, func_env.legacy_func);

   if (!eolian_function_is_class(funcid))
     {
        if (ftype == EOLIAN_PROP_GET || eolian_function_object_is_const(funcid))
           eina_strbuf_append(fparam, "const ");
        eina_strbuf_append_printf(fparam, "%s *obj", class_env.full_classname);
     }
   Eina_Strbuf *linedesc = eina_strbuf_new();
   const char *common_desc = eolian_function_description_get(funcid, EOLIAN_UNRESOLVED);
   const char *specific_desc = (ftype == EOLIAN_PROP_SET || ftype == EOLIAN_PROP_GET) ?
         eolian_function_description_get(funcid, ftype) : NULL;
   if (!common_desc && !specific_desc) eina_strbuf_append(linedesc, "No description supplied.");
   if (common_desc) eina_strbuf_append_printf(linedesc, "%s\n", common_desc);
   if (specific_desc) eina_strbuf_append(linedesc, specific_desc);
   if (eina_strbuf_length_get(linedesc))
     {
        eina_strbuf_replace_all(linedesc, "\n", "\n * ");
        eina_strbuf_replace_all(linedesc, " * \n", " *\n");
        eina_strbuf_prepend(linedesc," * ");
     }
   else
     {
        eina_strbuf_append(linedesc," *");
     }

   eina_strbuf_replace_all(fbody, "@#desc", eina_strbuf_string_get(linedesc));
   eina_strbuf_free(linedesc);

   itr = eolian_property_keys_get(funcid);
   EINA_ITERATOR_FOREACH(itr, data)
     {
        Eolian_Function_Parameter *param = data;
        const Eolian_Type *ptypet = eolian_parameter_type_get(param);
        const char *pname = eolian_parameter_name_get(param);
        const char *ptype = eolian_type_c_type_get(ptypet);
        const char *pdesc = eolian_parameter_description_get(param);
        leg_param_idx++;
        if (eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, ", ");
        eina_strbuf_append_printf(fparam, "%s%s %s",
              eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET)?"const":"",
              ptype, pname);
        eina_stringshare_del(ptype);
        eina_strbuf_append_printf(descparam, " * @param[in] %s %s\n", pname, pdesc?pdesc:"No description supplied.");
        if (eolian_parameter_is_nonull((Eolian_Function_Parameter*)data))
          {
             if (!flags)
               {
                  flags = eina_strbuf_new();
                  eina_strbuf_append_printf(flags, " EINA_ARG_NONNULL(%d", leg_param_idx);
               }
             else
                eina_strbuf_append_printf(flags, ", %d", leg_param_idx);
          }
     }
   eina_iterator_free(itr);
   if (!var_as_ret)
     {
       itr = eolian_function_parameters_get(funcid);
       EINA_ITERATOR_FOREACH(itr, data)
         {
            Eolian_Function_Parameter *param = data;
            const Eolian_Type *ptypet = eolian_parameter_type_get(param);
            const char *pname = eolian_parameter_name_get(param);
            const char *ptype = eolian_type_c_type_get(ptypet);
            const char *pdesc = eolian_parameter_description_get(param);
            Eolian_Parameter_Dir pdir = eolian_parameter_direction_get(param);
            const char *str_dir[] = {"in", "out", "inout"};
            Eina_Bool had_star = !!strchr(ptype, '*');
            if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM || pdir == EOLIAN_INOUT_PARAM);
            if (ftype == EOLIAN_PROP_GET) pdir = EOLIAN_OUT_PARAM;
            if (ftype == EOLIAN_PROP_SET) pdir = EOLIAN_IN_PARAM;
            leg_param_idx++;
            if (eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, ", ");
            eina_strbuf_append_printf(fparam, "%s%s%s%s%s",
                  eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET)?"const ":"",
                  ptype, had_star?"":" ", add_star?"*":"", pname);
            eina_stringshare_del(ptype);
            const char *dir_str = str_dir[(int)pdir];
            eina_strbuf_append_printf(descparam, " * @param[%s] %s %s\n", dir_str, pname, pdesc?pdesc:"No description supplied.");
            if (eolian_parameter_is_nonull((Eolian_Function_Parameter*)data))
              {
                 if (!flags)
                   {
                      flags = eina_strbuf_new();
                      eina_strbuf_append_printf(flags, " EINA_ARG_NONNULL(%d", leg_param_idx);
                   }
                 else
                    eina_strbuf_append_printf(flags, ", %d", leg_param_idx);
              }
         }
       eina_iterator_free(itr);
     }
   if (!eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, "void");
   if (flags) eina_strbuf_append_printf(flags, ")");

   if (rettypet) rettype = eolian_type_c_type_get(rettypet);

   eina_strbuf_replace_all(fbody, "@#params", eina_strbuf_string_get(fparam));
   eina_strbuf_replace_all(fbody, "@#list_desc_param", eina_strbuf_string_get(descparam));
   eina_strbuf_reset(fparam);
   eina_strbuf_append_printf(fparam, "%s%s%s",
         ret_const ? "const " : "",
         rettype ? rettype : "void",
         rettype && strchr(rettype, '*')?"":" ");
   eina_strbuf_replace_all(fbody, "@#type_return", eina_strbuf_string_get(fparam));
   if (eolian_function_return_is_warn_unused(funcid, ftype))
     {
        Eina_Bool no_nonull = !flags;
        if (no_nonull) flags = eina_strbuf_new();
        eina_strbuf_prepend(flags, " EINA_WARN_UNUSED_RESULT");
     }
   if (flags)
      eina_strbuf_replace_all(fbody, "@#flags", eina_strbuf_string_get(flags));
   eina_strbuf_replace_all(fbody, "@#flags", (eolian_function_return_is_warn_unused(funcid, ftype)) ? " EINA_WARN_UNUSED_RESULT" : "");
   eina_strbuf_append(buf, eina_strbuf_string_get(fbody));

   if (rettype) eina_stringshare_del(rettype);

end:
   eina_strbuf_free(flags);
   eina_strbuf_free(fbody);
   eina_strbuf_free(fparam);
   eina_strbuf_free(descparam);
}

static void
_eapi_func_generate(const Eolian_Class *class, const Eolian_Function *funcid, Eolian_Function_Type ftype, Eina_Strbuf *buf)
{
   _eolian_class_func_vars func_env;
   char tmpstr[0xFF];
   Eina_Bool var_as_ret = EINA_FALSE;
   const Eolian_Type *rettypet = NULL;
   const char *rettype = NULL;
   const char *retname = NULL;
   Eina_Bool ret_const = EINA_FALSE;
   Eina_Bool add_star = EINA_FALSE;
   Eina_Bool ret_is_void = EINA_FALSE;

   Eina_Strbuf *fbody = eina_strbuf_new();
   Eina_Strbuf *fparam = eina_strbuf_new();
   Eina_Strbuf *eoparam = eina_strbuf_new();

   Eina_Iterator *itr;
   void *data, *data2;

   _class_func_env_create(class, eolian_function_name_get(funcid), ftype, &func_env);
   rettypet = eolian_function_return_type_get(funcid, ftype);
   if (rettypet) rettype = eolian_type_c_type_get(rettypet);
   if (rettype && !strcmp(rettype, "void")) ret_is_void = EINA_TRUE;
   retname = "ret";
   if (ftype == EOLIAN_PROP_GET)
     {
        add_star = EINA_TRUE;
        if (!rettypet)
          {
             itr = eolian_function_parameters_get(funcid);
             /* We want to check if there is only one parameter */
             if (eina_iterator_next(itr, &data) && !eina_iterator_next(itr, &data2))
               {
                  Eolian_Function_Parameter *param = data;
                  rettypet = eolian_parameter_type_get(param);
                  retname = eolian_parameter_name_get(param);
                  var_as_ret = EINA_TRUE;
                  ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
               }
             eina_iterator_free(itr);
          }
     }

   if (func_env.legacy_func[0] == '\0') goto end;

   if (!rettype && rettypet) rettype = eolian_type_c_type_get(rettypet);

   if (rettype && (!ret_is_void))
     eina_strbuf_append(fbody, tmpl_eapi_body);
   else
     eina_strbuf_append(fbody, tmpl_eapi_body_void);

   if (!eolian_function_is_class(funcid))
     {
        if (ftype == EOLIAN_PROP_GET || eolian_function_object_is_const(funcid))
           eina_strbuf_append(fparam, "const ");
        eina_strbuf_append_printf(fparam, "%s *obj", class_env.full_classname);
        char cbuf[256];
        snprintf(cbuf, sizeof(cbuf), "(%s *)obj", class_env.full_classname);
        eina_strbuf_replace_all(fbody, "@#eo_obj", cbuf);
     }
   else
     {
        Eina_Strbuf *class_buf = eina_strbuf_new();
        _template_fill(class_buf, "@#CLASS_@#CLASSTYPE", class, NULL, NULL, EINA_TRUE);
        eina_strbuf_replace_all(fbody, "@#eo_obj", eina_strbuf_string_get(class_buf));
        eina_strbuf_free(class_buf);
     }
   eina_strbuf_replace_all(fbody, "@#eapi_func", func_env.legacy_func);
   eina_strbuf_replace_all(fbody, "@#eo_func", func_env.lower_eo_func);

   tmpstr[0] = '\0';

   itr = eolian_property_keys_get(funcid);
   EINA_ITERATOR_FOREACH(itr, data)
     {
        Eolian_Function_Parameter *param = data;
        const Eolian_Type *ptypet = eolian_parameter_type_get(param);
        const char *pname = eolian_parameter_name_get(param);
        const char *ptype = eolian_type_c_type_get(ptypet);
        if (eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, ", ");
        eina_strbuf_append_printf(fparam, "%s%s %s",
              eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET)?"const ":"",
              ptype, pname);
        eina_stringshare_del(ptype);
        if (eina_strbuf_length_get(eoparam)) eina_strbuf_append(eoparam, ", ");
        eina_strbuf_append_printf(eoparam, "%s", pname);
     }
   eina_iterator_free(itr);
   if (!var_as_ret)
   {
      itr = eolian_function_parameters_get(funcid);
      EINA_ITERATOR_FOREACH(itr, data)
        {
            Eolian_Function_Parameter *param = data;
            const Eolian_Type *ptypet = eolian_parameter_type_get(param);
            const char *pname = eolian_parameter_name_get(param);
            const char *ptype = eolian_type_c_type_get(ptypet);
            Eolian_Parameter_Dir pdir = eolian_parameter_direction_get(param);
            Eina_Bool had_star = !!strchr(ptype, '*');
            if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM || pdir == EOLIAN_INOUT_PARAM);
            if (eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, ", ");
            eina_strbuf_append_printf(fparam, "%s%s%s%s%s",
                  eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET)?"const ":"",
                  ptype, had_star?"":" ", add_star?"*":"", pname);
            eina_stringshare_del(ptype);
            if (eina_strbuf_length_get(eoparam)) eina_strbuf_append(eoparam, ", ");
            eina_strbuf_append_printf(eoparam, "%s", pname);
        }
      eina_iterator_free(itr);
   }
   if (!eina_strbuf_length_get(fparam)) eina_strbuf_append(fparam, "void");

   if (rettype && (!ret_is_void))
     {
        char tmp_ret_str[0xFF];
        sprintf (tmp_ret_str, "%s%s", ret_const?"const ":"", rettype);
             const Eolian_Expression *default_ret_val =
                eolian_function_return_default_value_get(funcid, ftype);
             const char *val_str = NULL;
             if (default_ret_val)
               {
                  Eolian_Value val = eolian_expression_eval_type
                    (default_ret_val, rettypet);
                  if (val.type)
                    val_str = eolian_expression_value_to_literal(&val);
               }
             Eina_Bool had_star = !!strchr(rettype, '*');
             sprintf (tmpstr, "   %s%s%s%s = %s;\n",
                   ret_const?"const ":"", rettype, had_star?"":" ", retname,
                   val_str?val_str:"0");

             eina_strbuf_replace_all(fbody, "@#ret_type", tmp_ret_str);
             eina_strbuf_replace_all(fbody, "@#ret_init_val", tmpstr);
     }

   eina_strbuf_replace_all(fbody, "@#full_params", eina_strbuf_string_get(fparam));
   eina_strbuf_replace_all(fbody, "@#eo_params", eina_strbuf_string_get(eoparam));

   eina_strbuf_replace_all(fbody, "@#ret_val", (rettype && !ret_is_void) ? retname : "");

   eina_strbuf_append(buf, eina_strbuf_string_get(fbody));

   if (rettype) eina_stringshare_del(rettype);

end:
   eina_strbuf_free(fbody);
   eina_strbuf_free(fparam);
   eina_strbuf_free(eoparam);
}

Eina_Bool
legacy_header_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   _class_env_create(class, NULL, &class_env);

   const char *desc = eolian_class_description_get(class);
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

   Eina_Iterator *itr = eolian_class_implements_get(class);
   if (itr)
     {
        const Eolian_Implement *impl;
        EINA_ITERATOR_FOREACH(itr, impl)
          {
             if (eolian_implement_class_get(impl) != class)
               continue;
             Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
             const Eolian_Function *fid = eolian_implement_function_get(impl, &ftype);
             switch (ftype)
               {
                case EOLIAN_PROP_GET: case EOLIAN_PROP_SET:
                  _eapi_decl_func_generate(class, fid, ftype, buf);
                  break;
                case EOLIAN_PROPERTY:
                  _eapi_decl_func_generate(class, fid, EOLIAN_PROP_SET, buf);
                  _eapi_decl_func_generate(class, fid, EOLIAN_PROP_GET, buf);
                  break;
                default:
                  _eapi_decl_func_generate(class, fid, EOLIAN_METHOD, buf);
                  break;
               }
          }
        eina_iterator_free(itr);
     }
   return EINA_TRUE;
}

Eina_Bool
legacy_source_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   Eina_Bool ret = EINA_FALSE;
   Eina_Iterator *itr;

   _class_env_create(class, NULL, &class_env);

   Eina_Strbuf *tmpbuf = eina_strbuf_new();
   Eina_Strbuf *str_bodyf = eina_strbuf_new();

   if ((itr = eolian_class_implements_get(class)))
     {
        const Eolian_Implement *impl;
        EINA_ITERATOR_FOREACH(itr, impl)
          {
             if (eolian_implement_class_get(impl) != class)
               continue;
             Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
             const Eolian_Function *fid = eolian_implement_function_get(impl, &ftype);
             switch (ftype)
               {
                case EOLIAN_PROP_GET: case EOLIAN_PROP_SET:
                  _eapi_func_generate(class, fid, ftype, str_bodyf);
                  break;
                case EOLIAN_PROPERTY:
                  _eapi_func_generate(class, fid, EOLIAN_PROP_SET, str_bodyf);
                  _eapi_func_generate(class, fid, EOLIAN_PROP_GET, str_bodyf);
                  break;
                default:
                  _eapi_func_generate(class, fid, EOLIAN_METHOD, str_bodyf);
                  break;
               }
          }
        eina_iterator_free(itr);
     }

   eina_strbuf_append(buf, eina_strbuf_string_get(str_bodyf));

   ret = EINA_TRUE;
   eina_strbuf_free(tmpbuf);
   eina_strbuf_free(str_bodyf);

   return ret;
}
