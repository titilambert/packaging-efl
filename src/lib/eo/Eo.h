#ifndef EO_H
#define EO_H

#include <stdarg.h>
#include <Eina.h>

#ifdef EAPI
# undef EAPI
#endif

#define EOLIAN

/* When used, it indicates that the function is an Eo API. */
#define EOAPI EAPI

#ifdef _WIN32
# ifdef EFL_EO_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_EO_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @page eo_main Eo
 *
 * @date 2012 (created)
 *
 * @section toc Table of Contents
 *
 * @li @ref eo_main_intro
 * @li @ref eo_main_compiling
 * @li @ref eo_main_next_steps
 * @li @ref eo_main_intro_example
 *
 * @section eo_main_intro Introduction
 *
 * The Eo generic object system. It was designed to be the base object
 * system for the EFL.

 * @section eo_main_compiling How to compile
 *
 * Eo is a library your application links to. The procedure for this is
 * very simple. You simply have to compile your application with the
 * appropriate compiler flags that the @c pkg-config script outputs. For
 * example:
 *
 * Compiling C or C++ files into object files:
 *
 * @verbatim
   gcc -c -o main.o main.c `pkg-config --cflags eo`
   @endverbatim
 *
 * Linking object files into a binary executable:
 *
 * @verbatim
   gcc -o my_application main.o `pkg-config --libs eo`
   @endverbatim
 *
 * See @ref pkgconfig
 *
 * @section eo_main_next_steps Next Steps
 *
 * After you understood what Eo is and installed it in your system
 * you should proceed understanding the programming interface.
 *
 * Recommended reading:
 *
 * @li @ref Eo_Class_Base
 * @li @ref Eo_Class
 * @li @ref Eo_Events
 * @li @ref Eo_Composite_Objects
 *
 * @section eo_main_intro_example Introductory Example
 *
 * @ref Eo_Tutorial

 *
 *
 * @addtogroup Eo
 * @{
 */

/**
 * @typedef Eo
 * The basic Object type.
 */
typedef struct _Eo_Opaque Eo;

/**
 * @typedef Eo_Class
 * The basic class type - should be removed, just for compat.
 * @deprecated
 */
typedef Eo Eo_Class;

/**
 * @typedef Eo_Callback_Priority
 *
 * Callback priority value. Range is -32k - 32k. The lower the number, the
 * higher the priority.
 *
 * @see EO_CALLBACK_PRIORITY_AFTER
 * @see EO_CALLBACK_PRIORITY_BEFORE
 * @see EO_CALLBACK_PRIORITY_DEFAULT
 */
typedef short Eo_Callback_Priority;

#ifdef EFL_BETA_API_SUPPORT

/**
 * @var _eo_class_creation_lock
 * This variable is used for locking purposes in the class_get function
 * defined in #EO_DEFINE_CLASS.
 * This is just to work around the fact that we need to init locks before
 * using them.
 * Don't touch it if you don't know what you are doing.
 * @internal
 */
EAPI extern Eina_Spinlock _eo_class_creation_lock;

/**
 * @internal
 * An enum representing the possible types of an Op.
 */
enum _Eo_Op_Type
{
   EO_OP_TYPE_INVALID = -1, /**< Invalid op. */
   EO_OP_TYPE_REGULAR = 0, /**< Regular op. */
   EO_OP_TYPE_CLASS, /**< Class op - a class op. Like static in Java/C++. */
};

/**
 * @internal
 * @typedef Eo_Op_Type
 * A convenience typedef for #_Eo_Op_Type.
 */
typedef enum _Eo_Op_Type Eo_Op_Type;

/**
 * @addtogroup Eo_Debug_Information Eo's Debug information helper.
 * @{
 */

/**
 * @struct _Eo_Dbg_Info
 * The structure for the debug info used by Eo.
 */
struct _Eo_Dbg_Info
{
   Eina_Stringshare *name; /**< The name of the part (stringshare). */
   Eina_Value value; /**< The value. */
};

/**
 * @var EO_DBG_INFO_TYPE
 * The Eina_Value_Type for the debug info.
 */
EAPI extern const Eina_Value_Type *EO_DBG_INFO_TYPE;

/**
 * @typedef Eo_Dbg_Info
 * A convenience typedef for #_Eo_Dbg_Info
 */
typedef struct _Eo_Dbg_Info Eo_Dbg_Info;

/**
 * Creates a list inside debug info list.
 * @param[in] list list where to append
 * @param[in] name name of the list
 * @return the new list
 */
static inline Eo_Dbg_Info *
EO_DBG_INFO_LIST_APPEND(Eo_Dbg_Info *list, const char *name)
{
   Eo_Dbg_Info *tmp = (Eo_Dbg_Info *)calloc(1, sizeof(*tmp));
   tmp->name = eina_stringshare_add(name);
   eina_value_list_setup(&(tmp->value), EO_DBG_INFO_TYPE);
   if (list)
     {
        eina_value_list_pappend(&(list->value), tmp);
     }
   return tmp;
}

/**
 * @def EO_DBG_INFO_APPEND
 * Creates a new debug info into a list
 * @param[in] LIST list where to append (Eo_Dbg_Info *)
 * @param[in] NAME name of the parameter (const char *)
 * @param[in] TYPE type of the parameter (Eina_Value_Type *)
 * @param[in] VALUE value of the parameter
 */
#define EO_DBG_INFO_APPEND(LIST, NAME, TYPE, VALUE) \
do { \
   Eo_Dbg_Info *List = LIST; \
   if (List) \
     { \
        Eo_Dbg_Info *Tmp = calloc(1, sizeof(*Tmp)); \
        Tmp->name = eina_stringshare_add(NAME); \
        eina_value_setup(&(Tmp->value), TYPE); \
        eina_value_set(&(Tmp->value), VALUE); \
        eina_value_list_pappend(&(List->value), Tmp); \
     } \
} while (0)

/**
 * Frees the Eo_Dbg_Info tree. (The whole tree recursively).
 * @param[in] info The tree to delete.
 */
EAPI void eo_dbg_info_free(Eo_Dbg_Info *info);

/**
 * @}
 */

/**
 * @typedef Eo_Op
 * The Eo operation type id.
 */
typedef unsigned int Eo_Op;

/**
 * @def EO_NOOP
 * A special #Eo_Op meaning "No operation".
 */
#define EO_NOOP ((Eo_Op) 0)

/**
 * @addtogroup Eo_Events Eo's Event Handling
 * @{
 */

/**
 * @struct _Eo_Event_Description
 * This struct holds the description of a specific event.
 */
struct _Eo_Event_Description
{
   const char *name; /**< name of the event. */
   const char *doc; /**< Explanation about the event. */

   Eina_Bool   unfreezable; /**< Eina_True if the event cannot be frozen */
};

/**
 * @typedef Eo_Event_Description
 * A convenience typedef for #_Eo_Event_Description
 */
typedef struct _Eo_Event_Description Eo_Event_Description;

/**
 * @def EO_EVENT_DESCRIPTION(name, doc)
 * An helper macro to help populating #Eo_Event_Description
 * @param name The name of the event.
 * @param doc Additional doc for the event.
 * @see Eo_Event_Description
 */
#define EO_EVENT_DESCRIPTION(name, doc) { name, doc, EINA_FALSE }

/**
 * @def EO_HOT_EVENT_DESCRIPTION(name, doc)
 * An helper macro to help populating #Eo_Event_Description and make
 * the event impossible to freeze.
 * @param name The name of the event.
 * @param doc Additional doc for the event.
 * @see Eo_Event_Description
 * @see EO_EVENT_DESCRIPTION
 */
#define EO_HOT_EVENT_DESCRIPTION(name, doc) { name, doc, EINA_TRUE }



/**
 * @}
 */

/**
 * @addtogroup Eo_Class Eo Class
 * @{
 */

/**
 * @def EO_DEFINE_CLASS(class_get_func_name, class_desc, parent_class, ...)
 * A convenience macro to be used for creating the class_get function. This
 * macro is fairly simple but should still be used as it'll let us improve
 * things easily.
 * @param class_get_func_name the name of the wanted class_get function name.
 * @param class_desc the class description.
 * @param parent_class The parent class for the function. Look at eo_class_new() for more information.
 * @param ... List of extensions. Look at eo_class_new() for more information.
 *
 * You must use this macro if you want thread safety in class creation.
 */
#define EO_DEFINE_CLASS(class_get_func_name, class_desc, parent_class, ...) \
EAPI const Eo_Class * \
class_get_func_name(void) \
{ \
   const Eo_Class *_tmp_parent_class; \
   static volatile char lk_init = 0; \
   static Eina_Spinlock _my_lock; \
   static const Eo_Class * volatile _my_class = NULL; \
   if (EINA_LIKELY(!!_my_class)) return _my_class; \
   \
   eina_spinlock_take(&_eo_class_creation_lock); \
   if (!lk_init) \
      eina_spinlock_new(&_my_lock); \
   if (lk_init < 2) eina_spinlock_take(&_my_lock); \
   if (!lk_init) \
      lk_init = 1; \
   else \
     { \
        if (lk_init < 2) eina_spinlock_release(&_my_lock); \
        eina_spinlock_release(&_eo_class_creation_lock); \
        return _my_class; \
     } \
   eina_spinlock_release(&_eo_class_creation_lock); \
   _tmp_parent_class = parent_class; \
   _my_class = eo_class_new(class_desc, _tmp_parent_class, __VA_ARGS__); \
   eina_spinlock_release(&_my_lock); \
   \
   eina_spinlock_take(&_eo_class_creation_lock); \
   eina_spinlock_free(&_my_lock); \
   lk_init = 2; \
   eina_spinlock_release(&_eo_class_creation_lock); \
   return _my_class; \
}


/**
 * An enum representing the possible types of an Eo class.
 */
enum _Eo_Class_Type
{
   EO_CLASS_TYPE_REGULAR = 0, /**< Regular class. */
   EO_CLASS_TYPE_REGULAR_NO_INSTANT, /**< Regular non instant-able class. */
   EO_CLASS_TYPE_INTERFACE, /**< Interface */
   EO_CLASS_TYPE_MIXIN /**< Mixin */
};

/**
 * @typedef Eo_Class_Type
 * A convenience typedef for #_Eo_Class_Type.
 */
typedef enum _Eo_Class_Type Eo_Class_Type;

/**
 * @def EO_VERSION
 * The current version of EO.
 */
#define EO_VERSION 2

typedef struct _Eo_Op_Description
{
   void *api_func;         /**< The EAPI function offering this op. */
#ifdef _WIN32
   const char *api_name;   /**< Full name of this API entry-point. Used to work around import indirection in DLL's. */
#endif
   void *func;             /**< The static function to call for the op. */
   Eo_Op op;               /**< The op. */
   Eo_Op_Type op_type;     /**< The type of the Op. */
   const char *doc;        /**< Explanation about the Op. */
} Eo_Op_Description;

/**
 * @struct _Eo_Class_Description
 * This struct holds the description of a class.
 * This description should be passed to eo_class_new.
 * Please use the #EO_CLASS_DESCRIPTION_OPS macro when populating it.
 */
struct _Eo_Class_Description
{
   unsigned int version; /**< The current version of eo, use #EO_VERSION */
   const char *name; /**< The name of the class. */
   Eo_Class_Type type; /**< The type of the class. */
   struct {
        Eo_Op_Description *descs; /**< The op descriptions array of size count. */
        size_t count; /**< Number of op descriptions. */
   } ops; /**< The ops description, should be filled using #EO_CLASS_DESCRIPTION_OPS (later sorted by Eo). */
   const Eo_Event_Description **events; /**< The event descriptions for this class. */
   size_t data_size; /**< The size of data (private + protected + public) this class needs per object. */
   void (*class_constructor)(Eo_Class *klass); /**< The constructor of the class. */
   void (*class_destructor)(Eo_Class *klass); /**< The destructor of the class. */
};

/**
 * @typedef Eo_Class_Description
 * A convenience typedef for #_Eo_Class_Description
 */
typedef struct _Eo_Class_Description Eo_Class_Description;

/**
 * @brief Create a new class.
 * @param desc the class description to create the class with.
 * @param parent the class to inherit from.
 * @param ... A NULL terminated list of extensions (interfaces, mixins and the classes of any composite objects).
 * @return The new class's handle on success, or NULL otherwise.
 *
 * You should use #EO_DEFINE_CLASS. It'll provide thread safety and other
 * features easily.
 *
 * @see #EO_DEFINE_CLASS
 */
EAPI const Eo_Class *eo_class_new(const Eo_Class_Description *desc, const Eo_Class *parent, ...);

/**
 * @brief Check if an object "is a" klass.
 * @param obj The object to check
 * @param klass The klass to check against.
 * @return @c EINA_TRUE if obj implements klass, @c EINA_FALSE otherwise.
 *
 * Notice: This function does not support composite objects.
 */
EAPI Eina_Bool eo_isa(const Eo *obj, const Eo_Class *klass);

/**
 * @brief Gets the name of the passed class.
 * @param klass the class to work on.
 * @return The class's name.
 *
 * @see eo_class_get()
 */
EAPI const char *eo_class_name_get(const Eo_Class *klass);

/**
 * @}
 */

/**
 * @brief Init the eo subsystem
 * @return @c EINA_TRUE on success.
 *
 * @see eo_shutfown()
 */
EAPI Eina_Bool eo_init(void);

/**
 * @brief Shutdown the eo subsystem
 * @return @c EINA_TRUE on success.
 *
 * @see eo_init()
 */
EAPI Eina_Bool eo_shutdown(void);

// computes size of Eo_Op_Description[]
#define EO_OP_DESC_SIZE(desc) (sizeof(desc)/sizeof(*desc) - 1)

// Helpers macro to help populating #Eo_Class_Description.
#define EO_CLASS_DESCRIPTION_NOOPS() { NULL, 0}
#define EO_CLASS_DESCRIPTION_OPS(op_descs) { op_descs, EO_OP_DESC_SIZE(op_descs) }

// to fetch internal function and object data at once
typedef struct _Eo_Op_Call_Data
{
   Eo       *obj;
   Eo_Class *klass;  // remove this not necessary in Eo_Hook_Call
   void     *func;
   void     *data;
} Eo_Op_Call_Data;

typedef void (*Eo_Hook_Call)(const Eo_Class *klass_id, const Eo *obj, const char *eo_func_name, void *func, ...);

EAPI extern Eo_Hook_Call eo_hook_call_pre;
EAPI extern Eo_Hook_Call eo_hook_call_post;

// to pass the internal function call to EO_FUNC_BODY (as Func parameter)
#define EO_FUNC_CALL(...) __VA_ARGS__

#define EO_HOOK_CALL_PREPARE(Hook, FuncName)                                \
     if (Hook)                                                          \
       Hook(call.klass, call.obj, FuncName, call.func);

#define EO_HOOK_CALL_PREPAREV(Hook, FuncName, ...)                          \
     if (Hook)                                                          \
       Hook(call.klass, call.obj, FuncName, call.func, __VA_ARGS__);

// cache OP id, get real fct and object data then do the call
#define EO_FUNC_COMMON_OP(Name, DefRet)                                 \
     Eo_Op_Call_Data call;                                              \
     Eina_Bool is_main_loop = eina_main_loop_is();                      \
     static Eo_Op op = EO_NOOP;                                         \
     if (op == EO_NOOP)                                                 \
       op = _eo_api_op_id_get((void*) Name, is_main_loop, __FILE__, __LINE__); \
     if (!_eo_call_resolve(#Name, op, &call, is_main_loop, __FILE__, __LINE__)) return DefRet; \
     _Eo_##Name##_func _func_ = (_Eo_##Name##_func) call.func;        \

// to define an EAPI function
#define EO_FUNC_BODY(Name, Ret, DefRet)                                \
  Ret                                                                   \
  Name(void)                                                            \
  {                                                                     \
     typedef Ret (*_Eo_##Name##_func)(Eo *, void *obj_data);           \
     Ret _r;                                                            \
     EO_FUNC_COMMON_OP(Name, DefRet);                                  \
     EO_HOOK_CALL_PREPARE(eo_hook_call_pre, #Name);                    \
     _r = _func_(call.obj, call.data);                                  \
     EO_HOOK_CALL_PREPARE(eo_hook_call_post, #Name);                    \
     return _r;                                                         \
  }

#define EO_VOID_FUNC_BODY(Name)					\
  void									\
  Name(void)                                                            \
  {                                                                     \
     typedef void (*_Eo_##Name##_func)(Eo *, void *obj_data);          \
     EO_FUNC_COMMON_OP(Name, );                                        \
     EO_HOOK_CALL_PREPARE(eo_hook_call_pre, #Name);                          \
     _func_(call.obj, call.data);                                       \
     EO_HOOK_CALL_PREPARE(eo_hook_call_post, #Name);                         \
  }

#define EO_FUNC_BODYV(Name, Ret, DefRet, Arguments, ...)               \
  Ret                                                                   \
  Name(__VA_ARGS__)                                                     \
  {                                                                     \
     typedef Ret (*_Eo_##Name##_func)(Eo *, void *obj_data, __VA_ARGS__); \
     Ret _r;                                                            \
     EO_FUNC_COMMON_OP(Name, DefRet);                                  \
     EO_HOOK_CALL_PREPAREV(eo_hook_call_pre, #Name, Arguments);              \
     _r = _func_(call.obj, call.data, Arguments);                       \
     EO_HOOK_CALL_PREPAREV(eo_hook_call_post, #Name, Arguments);             \
     return _r;                                                         \
  }

#define EO_VOID_FUNC_BODYV(Name, Arguments, ...)                       \
  void                                                                  \
  Name(__VA_ARGS__)                                                     \
  {                                                                     \
     typedef void (*_Eo_##Name##_func)(Eo *, void *obj_data, __VA_ARGS__); \
     EO_FUNC_COMMON_OP(Name, );                                        \
     EO_HOOK_CALL_PREPAREV(eo_hook_call_pre, #Name, Arguments);              \
     _func_(call.obj, call.data, Arguments);                            \
     EO_HOOK_CALL_PREPAREV(eo_hook_call_post, #Name, Arguments);             \
  }

// OP ID of an overriding function
#define EO_OP_OVERRIDE ((Eo_Op) -1)

#ifndef _WIN32
# define _EO_OP_API_ENTRY(a) a
#else
# define _EO_OP_API_ENTRY(a) a, #a
#endif

#define EO_OP_FUNC(_api, _private, _doc) { _EO_OP_API_ENTRY(_api), _private, EO_NOOP, EO_OP_TYPE_REGULAR, _doc }
#define EO_OP_CLASS_FUNC(_api, _private, _doc) { _EO_OP_API_ENTRY(_api), _private, EO_NOOP, EO_OP_TYPE_CLASS, _doc }
#define EO_OP_FUNC_OVERRIDE(_api, _private) { _EO_OP_API_ENTRY(_api), _private, EO_OP_OVERRIDE, EO_OP_TYPE_REGULAR, NULL }
#define EO_OP_CLASS_FUNC_OVERRIDE(_api, _private) { _EO_OP_API_ENTRY(_api), _private, EO_OP_OVERRIDE, EO_OP_TYPE_CLASS, NULL }
#define EO_OP_SENTINEL { _EO_OP_API_ENTRY(NULL), NULL, 0, EO_OP_TYPE_INVALID, NULL }

// returns the OP id corresponding to the given api_func
EAPI Eo_Op _eo_api_op_id_get(const void *api_func, Eina_Bool is_main_loop, const char *file, int line);

// gets the real function pointer and the object data
EAPI Eina_Bool _eo_call_resolve(const char *func_name, const Eo_Op op, Eo_Op_Call_Data *call, Eina_Bool is_main_loop, const char *file, int line);

// start of eo_do barrier, gets the object pointer and ref it, put it on the stask
  EAPI Eina_Bool _eo_do_start(const Eo *obj, const Eo_Class *cur_klass, Eina_Bool is_super, const char *file, const char *func, int line);

// end of the eo_do barrier, unref the obj, move the stack pointer
EAPI void _eo_do_end(const Eo **ojb);

#define EO_DO_CLEANUP __attribute__((cleanup(_eo_do_end)))

// eo object method calls batch,

#define _eo_do_common(eoid, clsid, is_super, ...)                       \
  ({                                                                    \
       const Eo *_eoid_ EO_DO_CLEANUP = eoid;                           \
       _eo_do_start(_eoid_, clsid, is_super, __FILE__, __FUNCTION__, __LINE__); \
       __VA_ARGS__;                                                     \
    })


#define eo_do(eoid, ...) _eo_do_common(eoid, NULL, EINA_FALSE, __VA_ARGS__)

#define eo_do_super(eoid, clsid, func) _eo_do_common(eoid, clsid, EINA_TRUE, func)

/*****************************************************************************/

/**
 * @brief Gets the class of the object.
 * @param obj The object to work on
 * @return The object's class.
 *
 * @see eo_class_name_get()
 */
EAPI const Eo_Class *eo_class_get(const Eo *obj);

/**
 * @def eo_error_set
 * @brief Notify eo that there was an error when constructing, destructing or calling a function of the object.
 * @param obj the object to work on.
 *
 * @see eo_error_get()
 */
#define eo_error_set(obj) eo_error_set_internal(obj, __FILE__, __LINE__)

/* @cond 0 */
EAPI void eo_error_set_internal(const Eo *obj, const char *file, int line);
/* @endcond */

/**
 * @def eo_add
 * @brief Create a new object with the default constructor.
 *
 * The object returned by this function always has at least 1 ref. If the object
 * has a parent, it returns with 1 ref, and even if it doesn't have a parent
 * it's returned with 1 ref. This is convenient in C.
 *
 * If you want a more "consistent" behaviour, take a look at #eo_add_ref.
 *
 * @param klass the class of the object to create.
 * @param parent the parent to set to the object.
 * @param ... The ops to run.
 * @return An handle to the new object on success, NULL otherwise.
 */
#define eo_add(klass, parent, ...) \
   ({ \
    const Eo_Class *_tmp_klass = klass; \
    Eo *_tmp_obj = _eo_add_internal_start(__FILE__, __LINE__, _tmp_klass, parent, EINA_FALSE); \
    eo_do(_tmp_obj, \
           eo_constructor(); \
           __VA_ARGS__; \
           _tmp_obj = eo_finalize(); \
          ); \
    _tmp_obj; \
    })

/**
 * @def eo_add_ref
 * @brief Create a new object with the default constructor.
 *
 * The object returned by this function has 1 ref for itself, 1 ref from the
 * parent (if exists) and possible other refs if were added during construction.
 *
 * @param klass the class of the object to create.
 * @param parent the parent to set to the object.
 * @param ... The ops to run.
 * @return An handle to the new object on success, NULL otherwise.
 */
#define eo_add_ref(klass, parent, ...) \
   ({ \
    const Eo_Class *_tmp_klass = klass; \
    Eo *_tmp_obj = _eo_add_internal_start(__FILE__, __LINE__, _tmp_klass, parent, EINA_TRUE); \
    eo_do(_tmp_obj, \
           eo_constructor(); \
           __VA_ARGS__; \
           _tmp_obj = eo_finalize(); \
          ); \
    _tmp_obj; \
    })

EAPI Eo * _eo_add_internal_start(const char *file, int line, const Eo_Class *klass_id, Eo *parent, Eina_Bool ref);

/**
 * @brief Get a pointer to the data of an object for a specific class.
 * @param obj the object to work on.
 * @param klass the klass associated with the data.
 * @return a pointer to the data.
 * @deprecated use eo_data_scope_get or eo_data_ref instead.
 */
EAPI void *eo_data_get(const Eo *obj, const Eo_Class *klass) EINA_DEPRECATED;

/**
 * @brief Get a pointer to the data of an object for a specific class.
 * The data reference count is not incremented. The pointer must be used only
 * in the scope of the function and its callees.
 * @param obj the object to work on.
 * @param klass the klass associated with the data.
 * @return a pointer to the data.
 *
 * @see eo_data_ref()
 * @see eo_data_unref()
 */
EAPI void *eo_data_scope_get(const Eo *obj, const Eo_Class *klass);

/**
 * @def eo_data_xref(obj, klass, ref_obj)
 * Use this macro if you want to associate a referencer object.
 * Convenience macro around eo_data_xref_internal()
 */
#define eo_data_xref(obj, klass, ref_obj) eo_data_xref_internal(__FILE__, __LINE__, obj, klass, ref_obj)

/**
 * @def eo_data_ref(obj, klass)
 * Use this macro if you don't want to associate a referencer object.
 * Convenience macro around eo_data_xref_internal()
 */
#define eo_data_ref(obj, klass) eo_data_xref_internal(__FILE__, __LINE__, obj, klass, (const Eo *)obj)

/**
 * @brief Get a pointer to the data of an object for a specific class and
 * increment the data reference count.
 * @param obj the object to work on.
 * @param klass the klass associated with the data.
 * @param ref_obj the object that references the data.
 * @param file the call's filename.
 * @param line the call's line number.
 * @return a pointer to the data.
 *
 * @see eo_data_xunref_internal()
 */
EAPI void *eo_data_xref_internal(const char *file, int line, const Eo *obj, const Eo_Class *klass, const Eo *ref_obj);

/**
 * @def eo_data_xunref(obj, data, ref_obj)
 * Use this function if you used eo_data_xref to reference the data.
 * Convenience macro around eo_data_xunref_internal()
 * @see eo_data_xref()
 */
#define eo_data_xunref(obj, data, ref_obj) eo_data_xunref_internal(obj, data, ref_obj)

/**
 * @def eo_data_unref(obj, data)
 * Use this function if you used eo_data_ref to reference the data.
 * Convenience macro around eo_data_unref_internal()
 * @see eo_data_ref()
 */
#define eo_data_unref(obj, data) eo_data_xunref_internal(obj, data, obj)

/**
 * @brief Decrement the object data reference count by 1.
 * @param obj the object to work on.
 * @param data a pointer to the data to unreference
 * @param file the call's filename.
 * @param line the call's line number.
 *
 * @see eo_data_xref_internal()
 */
EAPI void eo_data_xunref_internal(const Eo *obj, void *data, const Eo *ref_obj);

/**
 * @brief Increment the object's reference count by 1.
 * @param obj the object to work on.
 * @return The object passed.
 *
 * It's very easy to get a refcount leak and start leaking memory because
 * of a forgotten unref or an extra ref. That is why there are eo_xref
 * and eo_xunref that will make debugging easier in such a case.
 * Therefor, these functions should only be used in small scopes, i.e at the
 * start of some section in which the object may get freed, or if you know
 * what you are doing.
 *
 * @see eo_unref()
 * @see eo_ref_get()
 */
EAPI Eo *eo_ref(const Eo *obj);

/**
 * @brief Decrement the object's reference count by 1 and free it if needed.
 * @param obj the object to work on.
 *
 * @see eo_ref()
 * @see eo_ref_get()
 */
EAPI void eo_unref(const Eo *obj);

/**
 * @brief Return the ref count of the object passed.
 * @param obj the object to work on.
 * @return the ref count of the object.
 *
 * @see eo_ref()
 * @see eo_unref()
 */
EAPI int eo_ref_get(const Eo *obj);

/**
 * @brief Unrefs the object and reparents it to NULL.
 * @param obj the object to work on.
 *
 * Because eo_del() unrefs and reparents to NULL, it doesn't really delete the
 * object.
 *
 * @see eo_unref()
 * @see eo_parent_set()
 */
EAPI void eo_del(const Eo *obj);

/**
 * @def eo_xref(obj, ref_obj)
 * Convenience macro around eo_xref_internal()
 * @see eo_xref()
 */
#define eo_xref(obj, ref_obj) eo_xref_internal(__FILE__, __LINE__, obj, ref_obj)

/**
 * @brief Increment the object's reference count by 1 (and associate the ref with ref_obj)
 * @param obj the object to work on.
 * @param ref_obj the object that references obj.
 * @param file the call's filename.
 * @param line the call's line number.
 * @return The object passed (obj)
 *
 * People should not use this function, use #eo_xref instead.
 * A compile flag my make it and eobj_xunref() behave the same as eobj_ref()
 * and eobj_unref() respectively. So this should be used wherever possible.
 *
 * @see eo_xunref()
 */
EAPI Eo *eo_xref_internal(const char *file, int line, Eo *obj, const Eo *ref_obj);

/**
 * @brief Decrement the object's reference count by 1 and free it if needed. Will free the ref associated with ref_obj).
 * @param obj the object to work on.
 * @param ref_obj the object that references obj.
 *
 * This function only enforces the checks for object association. I.e don't rely
 * on it. If such enforces are compiled out, this function behaves the same as
 * eo_unref().
 *
 * @see eo_xref_internal()
 */
EAPI void eo_xunref(Eo *obj, const Eo *ref_obj);

/**
 * @brief Enable or disable the manual free feature.
 * @param obj the object to work on.
 * @param manual_free indicates if the free is manual (EINA_TRUE) or automatic (EINA_FALSE).
 *
 * The developer is in charge to call the function eo_manual_free to free the memory allocated for this object.
 *
 * Do not use, unless you really know what you are doing. It's used by Evas
 * because evas wants to keep its private data available even after the object
 * is deleted. Setting this to true makes Eo destruct the object but not free
 * the private data or the object itself.
 *
 * @see eo_manual_free()
 */
EAPI void eo_manual_free_set(Eo *obj, Eina_Bool manual_free);

/**
 * @brief Frees the object.
 * @param obj the object to work on.
 * This function must be called by the developer if the function
 * eo_manual_free_set has been called before with the parameter EINA_TRUE.
 * An error will be printed if this function is called when the manual
 * free option is not set to EINA_TRUE or the number of refs is not 0.
 * @return EINA_TRUE if successfully freed. EINA_FALSE otherwise.
 *
 * @see eo_manual_free_set()
 */
EAPI Eina_Bool eo_manual_free(Eo *obj);

/**
 * @brief Checks if the object was already descructed (only relevant for manual_free objects).
 * @param obj the object to check.
 * This function checks if the object was already destructed (but not alraedy
 * freed). It should only be used with objects that are supposed to be manually
 * freed, but not yet freed (but possibly destructed).
 *
 * @see eo_manual_free_set()
 */
EAPI Eina_Bool eo_destructed_is(const Eo *obj);

/**
 * @addtogroup Eo_Class_Class Eo's Class class.
 * @{
 */

#include "eo_abstract_class.eo.h"

/**
 * @}
 */

/**
 * @addtogroup Eo_Class_Base Eo's Base class.
 * @{
 */

/**
 * @typedef eo_key_data_free_func
 * Data free func prototype.
 */
typedef void (*eo_key_data_free_func)(void *);

/**
 * @def eo_weak_ref
 * @brief Reference a pointer to an Eo object
 * @param wref the pointer to use for the weak ref
 *
 * Same as eo_wref_add() with the difference that it call eo_do() itself. Checking
 * for *wref NULL and making sure that you pass the right pointer to both side of
 * eo_do().
 *
 * @see eo_weak_unref
 * @see eo_wref_add
 */
#define eo_weak_ref(wref)			   \
  do {						   \
    if (*wref) eo_do(*wref, eo_wref_add(wref));  \
  } while (0)

/**
 * @def eo_weak_unref
 * @brief Unreference a pointer to an Eo object
 * @param wref the pointer to use for the weak unref
 *
 * Same as eo_wref_del() with the difference that it call eo_do() itself. Checking
 * for *wref NULL and making sure that you pass the right pointer to both side of
 * eo_do().
 *
 * @see eo_weak_ref
 * @see eo_wref_del
 * @see eo_wref_del_safe
 */
#define eo_weak_unref(wref)			   \
  do {						   \
    if (*wref) eo_do(*wref, eo_wref_del(wref));  \
  } while (0)

/**
 * @def eo_wref_del_safe
 * @brief Delete the weak reference passed.
 * @param wref the weak reference to free.
 *
 * Same as eo_wref_del(), with the different that it's not called from eo_do()
 * so you don't need to check if *wref is not NULL.
 *
 * @see #eo_wref_del
 */
#define eo_wref_del_safe(wref) eo_weak_unref(wref)

/**
 * @addtogroup Eo_Events Eo's Event Handling
 * @{
 */

/**
 * Don't use.
 * @internal
 */
EAPI const Eo_Event_Description *eo_base_legacy_only_event_description_get(const char *_event_name);

/**
 * @def EO_CALLBACK_PRIORITY_BEFORE
 * Slightly more prioritized than default.
 */
#define EO_CALLBACK_PRIORITY_BEFORE -100
/**
 * @def EO_CALLBACK_PRIORITY_DEFAULT
 * Default callback priority level
 */
#define EO_CALLBACK_PRIORITY_DEFAULT 0
/**
 * @def EO_CALLBACK_PRIORITY_AFTER
 * Slightly less prioritized than default.
 */
#define EO_CALLBACK_PRIORITY_AFTER 100

/**
 * @def EO_CALLBACK_STOP
 * Stop calling callbacks for the even of which the callback was called for.
 * @see EO_CALLBACK_CONTINUE
 */
#define EO_CALLBACK_STOP EINA_FALSE

/**
 * @def EO_CALLBACK_CONTINUE
 * Continue calling callbacks for the even of which the callback was called for.
 * @see EO_CALLBACK_STOP
 */
#define EO_CALLBACK_CONTINUE EINA_TRUE

/**
 * @typedef Eo_Event_Cb
 *
 * An event callback prototype.
 *
 * @param data The user data registered with the callback.
 * @param obj The object which initiated the event.
 * @param desc The event's description.
 * @param event_info additional data passed with the event.
 * @return #EO_CALLBACK_STOP to stop calling additional callbacks for the event, #EO_CALLBACK_CONTINUE to continue.
 */
typedef Eina_Bool (*Eo_Event_Cb)(void *data, Eo *obj, const Eo_Event_Description *desc, void *event_info);

/**
 * @typedef Eo_Callback_Array_Item
 * A convenience typedef for #_Eo_Callback_Array_Item
 */
typedef struct _Eo_Callback_Array_Item Eo_Callback_Array_Item;

/**
 * @struct _Eo_Callback_Array_Item
 * An item in an array of callback desc/func.
 * @see eo_event_callback_array_add()
 */
struct _Eo_Callback_Array_Item
{
   const Eo_Event_Description *desc; /**< The event description. */
   Eo_Event_Cb func; /**< The callback function. */
};

#define EO_CALLBACKS_ARRAY_DEFINE(Name, ...)                            \
  static Eo_Callback_Array_Item *                                       \
  Name(void)                                                            \
  {                                                                     \
     static Eo_Callback_Array_Item internal[sizeof ((Eo_Callback_Array_Item[]) { __VA_ARGS__ }) / \
                                            sizeof (Eo_Callback_Array_Item) + \
                                            1] = { { 0, 0 } };          \
     if (internal[0].desc == NULL)                                      \
       {                                                                \
          memcpy(internal,                                              \
                 ((Eo_Callback_Array_Item[]) { __VA_ARGS__, { NULL, NULL } }), \
                 sizeof (Eo_Callback_Array_Item) +                      \
                 sizeof ((Eo_Callback_Array_Item[]) { __VA_ARGS__ }));  \
       }                                                                \
     return internal;                                                   \
  }

/**
 * @def eo_event_callback_add(obj, desc, cb, data)
 * Add a callback for an event.
 * @param[in] desc The description of the event to listen to.
 * @param[in] cb the callback to call.
 * @param[in] data additional data to pass to the callback.
 *
 * callbacks of the same priority are called in reverse order of creation.
 *
 * @see eo_event_callback_priority_add()
 */
#define eo_event_callback_add(desc, cb, data) \
   eo_event_callback_priority_add(desc, \
         EO_CALLBACK_PRIORITY_DEFAULT, cb, data)

/**
 * @def eo_event_callback_array_add(obj, desc, cb, data)
 * Add a callback array for an event.
 * @param[in] array an #Eo_Callback_Array_Item of events to listen to.
 * @param[in] data additional data to pass to the callback.
 *
 * callbacks of the same priority are called in reverse order of creation.
 *
 * @see eo_event_callback_array_priority_add()
 */
#define eo_event_callback_array_add(array, data) \
   eo_event_callback_array_priority_add(array, \
         EO_CALLBACK_PRIORITY_DEFAULT, data)

/**
 * @}
 */

/* XXX: Remove, for compat with the old names. */
#define EO_EV_CALLBACK_ADD EO_BASE_EVENT_CALLBACK_ADD
#define EO_EV_CALLBACK_DEL EO_BASE_EVENT_CALLBACK_DEL
#define EO_EV_DEL EO_BASE_EVENT_DEL

/**
 * @}
 */

#define _EO_BASE_EO_CLASS_TYPE
#include "eo_base.eo.h"

#define EO_CLASS EO_BASE_CLASS

#endif

/**
 * @}
 */

#ifdef __cplusplus
} 
#endif

#endif
