#ifndef __EOLIAN_DATABASE_H
#define __EOLIAN_DATABASE_H

#include <Eolian.h>

extern int _eolian_log_dom;
extern Eina_Prefix *_eolian_prefix;

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_eolian_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eolian_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eolian_log_dom, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_eolian_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eolian_log_dom, __VA_ARGS__)

extern Eina_Hash *_classes;
extern Eina_Hash *_aliases;
extern Eina_Hash *_structs;
extern Eina_Hash *_enums;
extern Eina_Hash *_globals;
extern Eina_Hash *_constants;
extern Eina_Hash *_classesf;
extern Eina_Hash *_aliasesf;
extern Eina_Hash *_structsf;
extern Eina_Hash *_enumsf;
extern Eina_Hash *_globalsf;
extern Eina_Hash *_constantsf;
extern Eina_Hash *_filenames; /* Hash: filename without extension -> full path */
extern Eina_Hash *_tfilenames;

/* a hash holding lists of deps */
extern Eina_Hash *_depclasses;

typedef struct _Eolian_Object
{
   const char *file;
   int line;
   int column;
} Eolian_Object;

typedef struct _Eolian_Dependency
{
   Eolian_Object base;
   Eina_Stringshare *filename;
   Eina_Stringshare *name;
} Eolian_Dependency;

struct _Eolian_Class
{
   Eolian_Object base;
   Eina_Stringshare *full_name;
   Eina_List *namespaces; /* List Eina_Stringshare * */
   Eina_Stringshare *name;
   Eolian_Class_Type type;
   Eina_Stringshare *description;
   Eina_Stringshare *legacy_prefix;
   Eina_Stringshare *eo_prefix;
   Eina_Stringshare *data_type;
   Eina_List *inherits; /* List Eina_Stringshare * */
   Eina_List *properties; /* List prop_name -> Eolian_Function */
   Eina_List *methods; /* List meth_name -> Eolian_Function */
   Eina_List *implements; /* List implements name -> Eolian_Implement */
   Eina_List *constructors; /* List constructors name -> Eolian_Constructor */
   Eina_List *events; /* List event_name -> Eolian_Event */
   Eina_Bool class_ctor_enable:1;
   Eina_Bool class_dtor_enable:1;
};

struct _Eolian_Function
{
   Eolian_Object base;
   Eolian_Object set_base;
   Eina_Stringshare *name;
   Eina_List *keys; /* list of Eolian_Function_Parameter */
   Eina_List *params; /* list of Eolian_Function_Parameter */
   Eolian_Function_Type type;
   Eolian_Object_Scope scope;
   Eolian_Type *get_ret_type;
   Eolian_Type *set_ret_type;
   Eolian_Expression *get_ret_val;
   Eolian_Expression *set_ret_val;
   Eolian_Implement *get_impl;
   Eolian_Implement *set_impl;
   Eina_Stringshare *get_legacy;
   Eina_Stringshare *set_legacy;
   Eina_Stringshare *common_description;
   Eina_Stringshare *get_description;
   Eina_Stringshare *set_description;
   Eina_Stringshare *get_return_comment;
   Eina_Stringshare *set_return_comment;
   Eina_Bool obj_is_const :1; /* True if the object has to be const. Useful for a few methods. */
   Eina_Bool get_virtual_pure :1;
   Eina_Bool set_virtual_pure :1;
   Eina_Bool get_auto :1;
   Eina_Bool set_auto :1;
   Eina_Bool get_empty :1;
   Eina_Bool set_empty :1;
   Eina_Bool get_return_warn_unused :1; /* also used for methods */
   Eina_Bool set_return_warn_unused :1;
   Eina_Bool get_only_legacy: 1;
   Eina_Bool set_only_legacy: 1;
   Eina_Bool is_class :1;
   Eina_Bool is_c_only :1;
   Eina_List *ctor_of;
   Eolian_Class *klass;
};

struct _Eolian_Function_Parameter
{
   Eolian_Object base;
   Eina_Stringshare *name;
   Eolian_Type *type;
   Eolian_Expression *value;
   Eina_Stringshare *description;
   Eolian_Parameter_Dir param_dir;
   Eina_Bool is_const_on_get :1; /* True if const in this the get property */
   Eina_Bool is_const_on_set :1; /* True if const in this the set property */
   Eina_Bool nonull :1; /* True if this argument cannot be NULL */
};

struct _Eolian_Type
{
   Eolian_Object base;
   Eolian_Type_Type type;
   Eina_List   *subtypes;
   Eolian_Type *base_type;
   Eina_Stringshare *name;
   Eina_Stringshare *full_name;
   Eina_List        *namespaces;
   Eina_Hash        *fields;
   Eina_List        *field_list;
   Eina_Stringshare *comment;
   Eina_Stringshare *legacy;
   Eina_Stringshare *freefunc;
   Eina_Bool is_const  :1;
   Eina_Bool is_own    :1;
   Eina_Bool is_extern :1;
};

struct _Eolian_Implement
{
   Eolian_Object base;
   const Eolian_Class *klass;
   const Eolian_Function *foo_id;
   Eina_Stringshare *full_name;
   Eina_Bool is_virtual :1;
   Eina_Bool is_prop_get :1;
   Eina_Bool is_prop_set :1;
   Eina_Bool is_auto: 1;
   Eina_Bool is_empty: 1;
};

struct _Eolian_Constructor
{
   Eolian_Object base;
   const Eolian_Class *klass;
   Eina_Stringshare *full_name;
   Eina_Bool is_optional: 1;
};

struct _Eolian_Event
{
   Eolian_Object base;
   Eina_Stringshare *name;
   Eina_Stringshare *comment;
   Eolian_Type *type;
   Eolian_Class *klass;
   int scope;
};

struct _Eolian_Struct_Type_Field
{
   Eina_Stringshare *name;
   Eolian_Object     base;
   Eolian_Type      *type;
   Eina_Stringshare *comment;
};

struct _Eolian_Enum_Type_Field
{
   Eina_Stringshare  *name;
   Eolian_Object      base;
   Eolian_Expression *value;
   Eina_Stringshare  *comment;
};

struct _Eolian_Expression
{
   Eolian_Object base;
   Eolian_Expression_Type type;
   union
   {
      struct
      {
         Eolian_Binary_Operator binop;
         Eolian_Expression *lhs;
         Eolian_Expression *rhs;
      };
      struct
      {
         Eolian_Unary_Operator unop;
         Eolian_Expression *expr;
      };
      Eolian_Value_Union value;
   };
};

struct _Eolian_Variable
{
   Eolian_Object         base;
   Eolian_Variable_Type  type;
   Eina_Stringshare     *name;
   Eina_Stringshare     *full_name;
   Eina_List            *namespaces;
   Eolian_Type          *base_type;
   Eolian_Expression    *value;
   Eina_Stringshare     *comment;
   Eina_Bool is_extern :1;
};

int database_init();
int database_shutdown();

char *database_class_to_filename(const char *cname);
Eina_Bool database_validate(void);
Eina_Bool database_class_name_validate(const char *class_name, const Eolian_Class **cl);

/* types */

void database_type_add(Eolian_Type *def);
void database_struct_add(Eolian_Type *tp);
void database_enum_add(Eolian_Type *tp);
void database_type_del(Eolian_Type *tp);
void database_typedef_del(Eolian_Type *tp);

void database_type_print(Eolian_Type *type);
void database_type_to_str(const Eolian_Type *tp, Eina_Strbuf *buf, const char *name);

/* expressions */

Eolian_Value database_expr_eval(const Eolian_Expression *expr, Eolian_Expression_Mask mask);
void database_expr_del(Eolian_Expression *expr);
void database_expr_print(Eolian_Expression *expr);

/* variables */

void database_var_del(Eolian_Variable *var);
void database_var_add(Eolian_Variable *var);

/* classes */
void database_class_del(Eolian_Class *cl);

/* functions */
void database_function_del(Eolian_Function *fid);
void database_function_constructor_add(Eolian_Function *func, const Eolian_Class *klass);

/* func parameters */
void database_parameter_del(Eolian_Function_Parameter *pdesc);

/* implements */
void database_implement_del(Eolian_Implement *impl);
void database_implement_constructor_add(Eolian_Implement *impl, const Eolian_Class *klass);

/* constructors */
void database_constructor_del(Eolian_Constructor *ctor);

/* events */
void database_event_del(Eolian_Event *event);


#endif
