#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <string.h>

#include "Eolian.h"
#include "eo_generator.h"
#include "common_funcs.h"

static _eolian_class_vars class_env;
/* Used to store the function names that will have to be appended
 * with __eolian during C generation. Needed when params have to
 * be initialized and for future features. */
static Eina_Hash *_funcs_params_init = NULL;

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
EO_DEFINE_CLASS(@#klasstype_get, &_@#class_class_desc, @#list_inheritNULL);\
";

static const char
tmpl_eo_obj_header[] = "\
#define @#CLASS_@#CLASSTYPE @#klasstype_get()\n\
\n\
const Eo_Class *@#klasstype_get(void) EINA_CONST;\n\
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
eo_fundef_generate(const Eolian_Class *class, const Eolian_Function *func, Eolian_Function_Type ftype, Eina_Strbuf *functext)
{
   _eolian_class_func_vars func_env;
   const char *str_dir[] = {"in", "out", "inout"};
   Eina_Iterator *itr;
   void *data, *data2;
   char *tmpstr = malloc(0x1FF);
   Eina_Bool var_as_ret = EINA_FALSE;
   const Eolian_Type *rettypet = NULL;
   const char *rettype = NULL;
   Eina_Bool ret_const = EINA_FALSE;
   Eolian_Object_Scope scope = eolian_function_scope_get(func);

   _class_func_env_create(class, eolian_function_name_get(func), ftype, &func_env);
   rettypet = eolian_function_return_type_get(func, ftype);
   if (ftype == EOLIAN_PROP_GET && !rettypet)
     {
        itr = eolian_function_parameters_get(func);
        /* We want to check if there is only one parameter */
        if (eina_iterator_next(itr, &data) && !eina_iterator_next(itr, &data2))
          {
             rettypet = eolian_parameter_type_get((Eolian_Function_Parameter*)data);
             var_as_ret = EINA_TRUE;
             ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
          }
        eina_iterator_free(itr);
     }
   Eina_Strbuf *str_func = eina_strbuf_new();
   if (scope == EOLIAN_SCOPE_PROTECTED)
      eina_strbuf_append_printf(str_func, "#ifdef %s_PROTECTED\n", class_env.upper_classname);

   eina_strbuf_append(str_func, tmpl_eo_funcdef_doxygen);
   eina_strbuf_append_printf(str_func, "EOAPI @#rettype %s(@#full_params);\n", func_env.lower_eo_func);

   if (scope == EOLIAN_SCOPE_PROTECTED)
      eina_strbuf_append_printf(str_func, "#endif\n");
   eina_strbuf_append_printf(str_func, "\n");

   Eina_Strbuf *linedesc = eina_strbuf_new();
   const char *common_desc = eolian_function_description_get(func, EOLIAN_UNRESOLVED);
   const char *specific_desc = (ftype == EOLIAN_PROP_SET || ftype == EOLIAN_PROP_GET) ?
         eolian_function_description_get(func, ftype) : NULL;
   if (!common_desc && !specific_desc) eina_strbuf_append(linedesc, "No description supplied.");
   if (common_desc) eina_strbuf_append_printf(linedesc, "%s\n", common_desc);
   if (specific_desc) eina_strbuf_append(linedesc, specific_desc);
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

   itr = eolian_property_keys_get(func);
   EINA_ITERATOR_FOREACH(itr, data)
     {
        Eolian_Function_Parameter *param = data;
        const Eolian_Type *ptypet = eolian_parameter_type_get(param);
        const char *pname = eolian_parameter_name_get(param);
        const char *ptype = eolian_type_c_type_get(ptypet);
        const char *pdesc = eolian_parameter_description_get(param);

        eina_strbuf_append_printf(str_pardesc, tmpl_eo_pardesc, "in", pname, pdesc?pdesc:"No description supplied.");

        if (eina_strbuf_length_get(str_par)) eina_strbuf_append(str_par, ", ");
        eina_strbuf_append_printf(str_par, "%s %s", ptype, pname);
        eina_stringshare_del(ptype);
     }
   eina_iterator_free(itr);

   if (!var_as_ret)
     {
        itr = eolian_function_parameters_get(func);
        EINA_ITERATOR_FOREACH(itr, data)
          {
             Eolian_Function_Parameter *param = data;
             const Eolian_Type *ptypet = eolian_parameter_type_get(param);
             const char *pname = eolian_parameter_name_get(param);
             const char *ptype = eolian_type_c_type_get(ptypet);
             const char *pdesc = eolian_parameter_description_get(param);
             Eina_Bool add_star = EINA_FALSE;
             Eolian_Parameter_Dir pdir = eolian_parameter_direction_get(param);

             Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
             if (ftype == EOLIAN_PROP_GET) {
                  add_star = EINA_TRUE;
                  pdir = EOLIAN_OUT_PARAM;
             }
             if (ftype == EOLIAN_PROP_SET) pdir = EOLIAN_IN_PARAM;
             if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM || pdir == EOLIAN_INOUT_PARAM);
             Eina_Bool had_star = !!strchr(ptype, '*');

             const char *dir_str = str_dir[(int)pdir];

             eina_strbuf_append_printf(str_pardesc, tmpl_eo_pardesc, dir_str, pname, pdesc?pdesc:"No description supplied.");

             if (eina_strbuf_length_get(str_par)) eina_strbuf_append(str_par, ", ");
             eina_strbuf_append_printf(str_par, "%s%s%s%s%s",
                   is_const?"const ":"",
                   ptype, had_star?"":" ", add_star?"*":"", pname);

             eina_stringshare_del(ptype);
          }
        eina_iterator_free(itr);
     }

   if (rettypet) rettype = eolian_type_c_type_get(rettypet);

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

   if (rettype) eina_stringshare_del(rettype);

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
eo_header_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   Eina_Iterator *itr;
   Eolian_Event *event;
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
   eina_strbuf_replace_all(str_hdr, "@#klasstype_get", eolian_class_c_get_function_name_get(class));

   Eina_Strbuf *str_ev = eina_strbuf_new();
   Eina_Strbuf *str_extrn_ev = eina_strbuf_new();
   Eina_Strbuf *tmpbuf = eina_strbuf_new();

   itr = eolian_class_events_get(class);
   EINA_ITERATOR_FOREACH(itr, event)
     {
        Eina_Stringshare *evname = eolian_event_c_name_get(event);
        const char *evdesc = eolian_event_description_get(event);
        Eolian_Object_Scope scope = eolian_event_scope_get(event);

        if (scope == EOLIAN_SCOPE_PRIVATE)
          continue;

        if (scope == EOLIAN_SCOPE_PROTECTED)
          {
             eina_strbuf_append_printf(str_ev, "\n#ifdef %s_PROTECTED\n", class_env.upper_classname);
             eina_strbuf_append_printf(str_extrn_ev, "#ifdef %s_PROTECTED\n", class_env.upper_classname);
          }
        else
          eina_strbuf_append_char(str_ev, '\n');

        if (!evdesc) evdesc = "No description";
        eina_strbuf_reset(tmpbuf);
        eina_strbuf_append(tmpbuf, evdesc);
        eina_strbuf_replace_all(tmpbuf, "\n", "\n * ");
        eina_strbuf_prepend(tmpbuf," * ");
        eina_strbuf_append_printf(str_ev, "/**\n%s\n */\n", eina_strbuf_string_get(tmpbuf));

        eina_strbuf_append_printf(str_ev, "#define %s (&(_%s))\n", evname, evname);
        eina_strbuf_append_printf(str_extrn_ev, "EOAPI extern const Eo_Event_Description _%s;\n", evname);
        eina_stringshare_del(evname);

        if (scope == EOLIAN_SCOPE_PROTECTED)
          {
             eina_strbuf_append(str_ev, "#endif\n");
             eina_strbuf_append(str_extrn_ev, "#endif\n");
          }
     }

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
                  eo_fundef_generate(class, fid, ftype, str_hdr);
                  break;
                case EOLIAN_PROPERTY:
                  eo_fundef_generate(class, fid, EOLIAN_PROP_SET, str_hdr);
                  eo_fundef_generate(class, fid, EOLIAN_PROP_GET, str_hdr);
                  break;
                default:
                  eo_fundef_generate(class, fid, EOLIAN_UNRESOLVED, str_hdr);
                  break;
               }
          }
        eina_iterator_free(itr);
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
eo_bind_func_generate(const Eolian_Class *class, const Eolian_Function *funcid, Eolian_Function_Type ftype, Eina_Strbuf *buf, Eolian_Implement *impl, _eolian_class_vars *impl_env)
{
   _eolian_class_func_vars func_env;
   const char *suffix = "";
   Eina_Bool var_as_ret = EINA_FALSE;
   const Eolian_Type *rettypet = NULL;
   const char *rettype = NULL;
   Eina_Bool ret_const = EINA_FALSE;
   Eina_Bool add_star = EINA_FALSE;
   Eina_Iterator *itr;
   void *data, *data2;
   const Eolian_Expression *default_ret_val = NULL;
   Eina_Bool is_empty = impl ? eolian_implement_is_empty(impl) : eolian_function_is_empty(funcid, ftype);
   Eina_Bool is_auto = impl ? eolian_implement_is_auto(impl) : eolian_function_is_auto(funcid, ftype);
   if (ftype != EOLIAN_PROP_GET && ftype != EOLIAN_PROP_SET) ftype = eolian_function_type_get(funcid);

   Eina_Bool need_implementation = EINA_TRUE;
   if (!impl_env && eolian_function_is_virtual_pure(funcid, ftype)) need_implementation = EINA_FALSE;

   Eina_Strbuf *fbody = eina_strbuf_new();
   Eina_Strbuf *va_args = eina_strbuf_new();
   Eina_Strbuf *params = eina_strbuf_new(); /* only variables names */
   Eina_Strbuf *full_params = eina_strbuf_new(); /* variables types + names */
   Eina_Strbuf *params_init = eina_strbuf_new(); /* init of variables to default */

   rettypet = eolian_function_return_type_get(funcid, ftype);
   if (rettypet)
     {
        is_auto = EINA_FALSE; /* We block auto when the function has to return something */
        default_ret_val = eolian_function_return_default_value_get(funcid, ftype);
     }
   if (ftype == EOLIAN_PROP_GET)
     {
        suffix = "_get";
        add_star = EINA_TRUE;
        if (!rettypet)
          {
             itr = eolian_function_parameters_get(funcid);
             /* We want to check if there is only one parameter */
             if (eina_iterator_next(itr, &data) && !eina_iterator_next(itr, &data2))
               {
                  Eolian_Function_Parameter *param = data;
                  rettypet = eolian_parameter_type_get(param);
                  var_as_ret = EINA_TRUE;
                  ret_const = eolian_parameter_const_attribute_get(data, EINA_TRUE);
                  default_ret_val = eolian_parameter_default_value_get(param);
               }
             eina_iterator_free(itr);
          }
     }
   if (ftype == EOLIAN_PROP_SET)
     {
        suffix = "_set";
     }

   itr = eolian_property_keys_get(funcid);
   EINA_ITERATOR_FOREACH(itr, data)
     {
        Eolian_Function_Parameter *param = data;
        const Eolian_Type *ptypet = eolian_parameter_type_get(param);
        const char *pname = eolian_parameter_name_get(param);
        const char *ptype = eolian_type_c_type_get(ptypet);
        Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
        if (eina_strbuf_length_get(params)) eina_strbuf_append(params, ", ");
        eina_strbuf_append_printf(params, "%s", pname);
        eina_strbuf_append_printf(full_params, ", %s%s %s%s",
              is_const?"const ":"",
              ptype, pname, is_empty || is_auto?" EINA_UNUSED":"");
        eina_stringshare_del(ptype);
     }
   eina_iterator_free(itr);
   if (!var_as_ret)
     {
        itr = eolian_function_parameters_get(funcid);
        EINA_ITERATOR_FOREACH(itr, data)
          {
             Eolian_Function_Parameter *param = data;
             const Eolian_Expression *dflt_value = eolian_parameter_default_value_get(param);
             const Eolian_Type *ptypet = eolian_parameter_type_get(param);
             const char *pname = eolian_parameter_name_get(param);
             const char *ptype = eolian_type_c_type_get(ptypet);
             Eolian_Parameter_Dir pdir = eolian_parameter_direction_get(param);
             Eina_Bool is_const = eolian_parameter_const_attribute_get(data, ftype == EOLIAN_PROP_GET);
             Eina_Bool had_star = !!strchr(ptype, '*');
             if (ftype == EOLIAN_UNRESOLVED || ftype == EOLIAN_METHOD) add_star = (pdir == EOLIAN_OUT_PARAM || pdir == EOLIAN_INOUT_PARAM);
             if (eina_strbuf_length_get(params)) eina_strbuf_append(params, ", ");
             eina_strbuf_append_printf(params, "%s", pname);
             eina_strbuf_append_printf(full_params, ", %s%s%s%s%s%s",
                   is_const?"const ":"",
                   ptype, had_star?"":" ", add_star?"*":"", pname, is_empty && !dflt_value ?" EINA_UNUSED":"");
             if (is_auto)
               {
                  if (ftype == EOLIAN_PROP_SET)
                    {
                       eina_strbuf_append_printf(params_init,
                             "   %s = pd->%s;\n", pname, pname);
                    }
                  else
                    {
                       eina_strbuf_append_printf(params_init,
                             "   if (%s) *%s = pd->%s;\n", pname, pname, pname);
                    }
               }
             else {
                  if (ftype != EOLIAN_PROP_SET)
                    {
                       if (dflt_value)
                         {
                            const char *val_str = NULL;
                            Eolian_Value val = eolian_expression_eval
                               (dflt_value, EOLIAN_MASK_ALL);
                            if (val.type)
                              {
                                 val_str = eolian_expression_value_to_literal(&val);
                                 eina_strbuf_append_printf(params_init,
                                       "   if (%s) *%s = %s;",
                                       pname, pname, val_str);
                                 if (eolian_expression_type_get(dflt_value) == EOLIAN_EXPR_ENUM)
                                   {
                                      Eina_Stringshare *string = eolian_expression_serialize(dflt_value);
                                      eina_strbuf_append_printf(params_init, " /* %s */", string);
                                      eina_stringshare_del(string);
                                   }
                                 eina_strbuf_append_printf(params_init, "\n");
                              }
                         }
                    }
               }
             eina_stringshare_del(ptype);
          }
        eina_iterator_free(itr);
     }

   if (rettypet) rettype = eolian_type_c_type_get(rettypet);

   if (need_implementation)
     {
        Eina_Strbuf *ret_param = eina_strbuf_new();
        eina_strbuf_append_printf(fbody, "\n");
        /* Generation of the user function prototype declaration - not needed when @auto and @empty are indicated */
        if (!is_empty && !is_auto)
          {
             eina_strbuf_append_printf(fbody, "%s%s _%s%s%s_%s%s(Eo *obj, @#Datatype_Data *pd%s);\n\n",
                   ret_const?"const ":"", rettype?rettype:"void",
                   class_env.lower_classname,
                   impl_env?"_":"",
                   impl_env?impl_env->lower_classname:"",
                   eolian_function_name_get(funcid), suffix,
                   eina_strbuf_string_get(full_params));
          }

        if (is_empty || is_auto || eina_strbuf_length_get(params_init))
          {
             /* We need to give the internal function name to Eo. We use this hash table as indication */
             eina_hash_add(_funcs_params_init,
                   eina_stringshare_add(eolian_function_name_get(funcid)), (void *)ftype);
             /* Generation of the intermediate function __eolian_... */
             eina_strbuf_append_printf(fbody, "static %s%s __eolian_%s%s%s_%s%s(Eo *obj%s, @#Datatype_Data *pd%s%s)\n{\n",
                   ret_const?"const ":"", rettype?rettype:"void",
                   class_env.lower_classname,
                   impl_env?"_":"",
                   impl_env?impl_env->lower_classname:"",
                   eolian_function_name_get(funcid), suffix,
                   is_empty || is_auto?" EINA_UNUSED":"",
                   is_empty || (is_auto && !eina_strbuf_length_get(params_init))?" EINA_UNUSED":"",
                   eina_strbuf_string_get(full_params));
          }
        if (eina_strbuf_length_get(params_init))
          {
             eina_strbuf_append_printf(fbody, "%s", eina_strbuf_string_get(params_init));
          }
        if (is_auto || is_empty)
          {
             if (rettype)
               {
                  /* return for auto and empty */
                  const char *val_str = NULL;
                  if (default_ret_val)
                    {
                       Eolian_Value val = eolian_expression_eval
                          (default_ret_val, EOLIAN_MASK_ALL);
                       if (val.type)
                          val_str = eolian_expression_value_to_literal(&val);
                    }
                  eina_strbuf_append_printf(fbody, "   return %s;\n", val_str?val_str:"0");
               }
             eina_strbuf_append_printf(fbody, "}\n\n");
          }
        else if (eina_strbuf_length_get(params_init))
          {
             /* Generation of the user function invocation, e.g return _user_foo(obj, pd, ...) */
             eina_strbuf_append_printf(fbody, "   %s_%s%s%s_%s%s(obj, pd, %s);\n}\n\n",
                   rettype?"return ":"",
                   class_env.lower_classname,
                   impl_env?"_":"",
                   impl_env?impl_env->lower_classname:"",
                   eolian_function_name_get(funcid), suffix,
                   eina_strbuf_string_get(params));
          }
        eina_strbuf_free(ret_param);
     }
   if (!impl_env)
     {
        Eina_Strbuf *eo_func_decl = eina_strbuf_new();
        Eina_Bool has_params = EINA_FALSE;

        itr = eolian_property_keys_get(funcid);
        has_params |= (eina_iterator_next(itr, &data));
        eina_iterator_free(itr);

        if (!has_params && !var_as_ret)
          {
             itr = eolian_function_parameters_get(funcid);
             has_params |= (eina_iterator_next(itr, &data));
             eina_iterator_free(itr);
          }
        Eina_Bool ret_is_void = (!rettype || !strcmp(rettype, "void"));
        _class_func_env_create(class, eolian_function_name_get(funcid), ftype, &func_env);
        eina_strbuf_append_printf(eo_func_decl,
              "EOAPI EO_%sFUNC_BODY%s(%s",
              ret_is_void?"VOID_":"", has_params?"V":"",
              func_env.lower_eo_func);
        if (!ret_is_void)
          {
             const char *val_str = NULL;
             if (default_ret_val)
               {
                  Eolian_Value val = eolian_expression_eval
                    (default_ret_val, EOLIAN_MASK_ALL);
                  if (val.type)
                    val_str = eolian_expression_value_to_literal(&val);
               }
             eina_strbuf_append_printf(eo_func_decl, ", %s%s, %s",
                   ret_const ? "const " : "", rettype,
                   val_str?val_str:"0");
             if (val_str && eolian_expression_type_get(default_ret_val) == EOLIAN_EXPR_ENUM)
               {
                  Eina_Stringshare *string = eolian_expression_serialize(default_ret_val);
                  eina_strbuf_append_printf(eo_func_decl, " /* %s */", string);
                  eina_stringshare_del(string);
               }
          }
        if (has_params)
          {
             eina_strbuf_replace_all(full_params, " EINA_UNUSED", "");
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
        Eina_Bool is_cf = eolian_function_is_class(funcid);
        const char *data_type = eolian_class_data_type_get(class);
        if (is_cf || (data_type && !strcmp(data_type, "null")))
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

   if (rettype) eina_stringshare_del(rettype);

   eina_strbuf_free(va_args);
   eina_strbuf_free(full_params);
   eina_strbuf_free(params_init);
   eina_strbuf_free(params);
   eina_strbuf_free(fbody);
   return EINA_TRUE;
}

static Eina_Bool
eo_op_desc_generate(const Eolian_Class *class, const Eolian_Function *fid, Eolian_Function_Type ftype,
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
   const char *class_str = "";
   if (eolian_function_is_class(fid)) class_str = "CLASS_";
   eina_strbuf_append_printf(buf, "\n     EO_OP_%sFUNC(%s, ", class_str, func_env.lower_eo_func);
   if (!is_virtual_pure)
     {
        Eolian_Function_Type ftype2 = (Eolian_Function_Type) eina_hash_find(_funcs_params_init, funcname);
        eina_strbuf_append_printf(buf, "%s_%s_%s%s, \"%s\"),",
              ftype == ftype2?"__eolian":"",
              class_env.lower_classname, funcname, suffix, desc);
     }
   else
      eina_strbuf_append_printf(buf, "NULL, \"%s\"),", desc);

   return EINA_TRUE;
}

static Eina_Bool
eo_source_beginning_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   Eina_Iterator *itr;

   Eina_Strbuf *tmpbuf = eina_strbuf_new();

   Eolian_Event *event;
   itr = eolian_class_events_get(class);
   EINA_ITERATOR_FOREACH(itr, event)
     {
        Eina_Stringshare *evname = eolian_event_c_name_get(event);
        const char *evdesc = eolian_event_description_get(event);
        char *evdesc_line1 = _source_desc_get(evdesc);

        eina_strbuf_append_printf(tmpbuf,
                                  "EOAPI const Eo_Event_Description _%s =\n   EO_EVENT_DESCRIPTION(\"%s\", \"%s\");\n",
                                  evname, eolian_event_name_get(event), evdesc_line1);
        eina_stringshare_del(evname);
        free(evdesc_line1);
     }
   eina_iterator_free(itr);

   eina_strbuf_append(buf, eina_strbuf_string_get(tmpbuf));

   eina_strbuf_free(tmpbuf);
   return EINA_TRUE;
}

static void
_desc_generate(const Eolian_Class *class, const Eolian_Function *fid, Eolian_Function_Type ftype, Eina_Strbuf *tmpbuf, Eina_Strbuf *str_op)
{
   const char *funcname = eolian_function_name_get(fid);
   char tmpstr[256];
   snprintf(tmpstr, sizeof(tmpstr), "%s%s", funcname, (ftype == EOLIAN_PROP_SET)
     ? "_set" : ((ftype == EOLIAN_PROP_GET) ? "_get" : ""));

   char *desc = _source_desc_get(eolian_function_description_get(fid, ftype));
   eo_op_desc_generate(class, fid, ftype, desc, tmpbuf);
   eina_strbuf_append(str_op, eina_strbuf_string_get(tmpbuf));
   free(desc);
}

static Eina_Bool
eo_source_end_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   Eina_Bool ret = EINA_FALSE;
   Eina_Iterator *itr;
   Eolian_Implement *impl_desc;
   Eolian_Event *event;
   const char *inherit_name;

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
   eina_strbuf_replace_all(str_end, "@#klasstype_get", eolian_class_c_get_function_name_get(class));

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
   itr = eolian_class_implements_get(class);
   EINA_ITERATOR_FOREACH(itr, impl_desc)
     {
        _eolian_class_vars impl_env;
        char implname[0xFF];
        const Eolian_Class *impl_class = NULL;
        Eolian_Function_Type ftype;
        const Eolian_Function *fnid = NULL;
        const char *funcname = NULL;
        const char *rets;

        const char *names[] = { "", "getter ", "setter " };

        impl_class = eolian_implement_class_get(impl_desc);

        if (impl_class)
          {
             char *tp = implname;
             if (impl_class == class)
               continue;
             fnid = eolian_implement_function_get(impl_desc, &ftype);
             _class_env_create(impl_class, NULL, &impl_env);
             funcname = eolian_function_name_get(fnid);

             sprintf(implname, "%s_%s_%s",
                   eolian_implement_is_auto(impl_desc) || eolian_implement_is_empty(impl_desc)?
                   "__eolian":"",
                   class_env.full_classname, impl_env.full_classname);
             eina_str_tolower(&tp);
          }

        if (!fnid)
          {
             const char *name = names[eolian_implement_is_prop_get(impl_desc)
                                   | (eolian_implement_is_prop_set(impl_desc) << 1)];
             ERR ("Failed to generate implementation of %s%s - missing form super class",
                   name, eolian_implement_full_name_get(impl_desc));
             goto end;
          }

        rets = eolian_function_full_c_name_get(fnid);

        const char *class_str = "";
        if (eolian_function_is_class(fnid)) class_str = "CLASS_";

        switch (ftype)
          {
           case EOLIAN_PROP_SET: case EOLIAN_PROP_GET: case EOLIAN_PROPERTY:
              if (ftype != EOLIAN_PROP_GET)
                {
                   eina_strbuf_append_printf(str_op, "\n     EO_OP_%sFUNC_OVERRIDE(%s_set, %s_%s_set),",
                         class_str, rets, implname, funcname);
                   eo_bind_func_generate(class, fnid, EOLIAN_PROP_SET, str_bodyf, impl_desc, &impl_env);
                }

              if (ftype != EOLIAN_PROP_SET)
                {
                   eina_strbuf_append_printf(str_op, "\n     EO_OP_%sFUNC_OVERRIDE(%s_get, %s_%s_get),",
                         class_str, rets, implname, funcname);
                   eo_bind_func_generate(class, fnid, EOLIAN_PROP_GET, str_bodyf, impl_desc, &impl_env);
                }
              break;
           default:
              eina_strbuf_append_printf(str_op, "\n     EO_OP_%sFUNC_OVERRIDE(%s, %s_%s),",
                    class_str, rets, implname, funcname);
              eo_bind_func_generate(class, fnid, ftype, str_bodyf, impl_desc, &impl_env);
              break;
          }
        eina_stringshare_del(rets);
     }
   eina_iterator_free(itr);

   if ((itr = eolian_class_implements_get(class)))
     {
        const Eolian_Implement *impl;
        EINA_ITERATOR_FOREACH(itr, impl)
          {
             if (eolian_implement_class_get(impl) != class)
               continue;
             Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
             const Eolian_Function *fid = eolian_implement_function_get(impl, &ftype);

             Eina_Bool prop_read  = (ftype == EOLIAN_PROPERTY || ftype == EOLIAN_PROP_GET);
             Eina_Bool prop_write = (ftype == EOLIAN_PROPERTY || ftype == EOLIAN_PROP_SET);

             if (!prop_read && !prop_write && !eolian_function_is_legacy_only(fid, EOLIAN_METHOD))
               _desc_generate(class, fid, EOLIAN_METHOD, tmpbuf, str_op);
             if (prop_write && !eolian_function_is_legacy_only(fid, EOLIAN_PROP_SET))
               _desc_generate(class, fid, EOLIAN_PROP_SET, tmpbuf, str_op);
             if (prop_read && !eolian_function_is_legacy_only(fid, EOLIAN_PROP_GET))
               _desc_generate(class, fid, EOLIAN_PROP_GET, tmpbuf, str_op);
          }
        eina_iterator_free(itr);
     }

   itr = eolian_class_events_get(class);
   EINA_ITERATOR_FOREACH(itr, event)
     {
        Eina_Stringshare *evname = eolian_event_c_name_get(event);
        eina_strbuf_append_printf(str_ev, "\n     %s,", evname);
        eina_stringshare_del(evname);
     }
   eina_iterator_free(itr);

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

   eina_strbuf_reset(tmpbuf);
   itr = eolian_class_inherits_get(class);
   EINA_ITERATOR_FOREACH(itr, inherit_name)
     {
        const Eolian_Class *inherit_class = eolian_class_get_by_name(inherit_name);
        _eolian_class_vars inherit_env;
        _class_env_create(inherit_class, NULL, &inherit_env);
        eina_strbuf_append_printf(tmpbuf, "%s_%s, ", inherit_env.upper_classname,
                                  inherit_env.upper_classtype);
     }
   eina_iterator_free(itr);

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
eo_source_generate(const Eolian_Class *class, Eina_Strbuf *buf)
{
   Eina_Bool ret = EINA_FALSE;
   Eina_Iterator *itr = NULL;

   Eina_Strbuf *str_bodyf = eina_strbuf_new();

   _class_env_create(class, NULL, &class_env);
   _funcs_params_init = eina_hash_stringshared_new(NULL);

   if (!eo_source_beginning_generate(class, buf)) goto end;

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
                  if (!eo_bind_func_generate(class, fid, ftype, str_bodyf, NULL, NULL))
                    goto end;
                  break;
                case EOLIAN_PROPERTY:
                  if (!eo_bind_func_generate(class, fid, EOLIAN_PROP_SET, str_bodyf, NULL, NULL))
                    goto end;
                  if (!eo_bind_func_generate(class, fid, EOLIAN_PROP_GET, str_bodyf, NULL, NULL))
                    goto end;
                  break;
                default:
                  if (!eo_bind_func_generate(class, fid, EOLIAN_UNRESOLVED, str_bodyf, NULL, NULL))
                    goto end;
                  break;
               }
          }
        eina_iterator_free(itr);
        itr = NULL;
     }

   eina_strbuf_append(buf, eina_strbuf_string_get(str_bodyf));
   eina_strbuf_reset(str_bodyf);

   if (!eo_source_end_generate(class, buf)) goto end;

   ret = EINA_TRUE;
end:
   if (itr) eina_iterator_free(itr);
   eina_hash_free(_funcs_params_init);
   _funcs_params_init = NULL;
   eina_strbuf_free(str_bodyf);
   return ret;
}

