
#ifndef EOLIAN_CXX_EO_TYPES_HH
#define EOLIAN_CXX_EO_TYPES_HH

#include <algorithm>
#include <string>
#include <vector>
#include <cassert>

namespace efl { namespace eolian {

struct eo_constructor;
struct eo_parameter;
struct eo_function;
struct eo_event;

typedef std::vector<std::string> ancestors_container_type;
typedef std::vector<std::string> includes_container_type;
typedef std::vector<eo_constructor> constructors_container_type;
typedef std::vector<eo_function> functions_container_type;
typedef std::vector<eo_parameter> parameters_container_type;
typedef std::vector<eo_event> events_container_type;

struct eolian_type
{
   enum category_type
   {
     unknown_, simple_, complex_, callback_
   };

   eolian_type()
     : native("")
     , category(unknown_)
     , is_const(false)
     , is_own(false)
     , is_class(false)
     , binding()
     , includes()
   {}

   eolian_type(std::string native_,
               category_type category_,
               bool is_const_,
               bool is_own_,
               bool is_class_,
               std::string binding_,
               includes_container_type includes_)
     : native(native_)
     , category(category_)
     , is_const(is_const_)
     , is_own(is_own_)
     , is_class(is_class_)
     , binding(binding_)
     , includes(includes_)
   {
      assert(!native.empty());
      assert(category != unknown_);
   }

   eolian_type(std::string native_,
               category_type category_,
               includes_container_type const& includes_)
     : eolian_type(native_, category_, false, false, false, "", includes_)
   {
      assert(category == callback_);
   }

   std::string native;
   category_type category;
   bool is_const;
   bool is_own;
   bool is_class;
   std::string binding;
   includes_container_type includes;
};

typedef std::vector<eolian_type> eolian_type_container;

struct eolian_type_instance
{
  eolian_type_instance()
    : is_out(false)
    , is_nonull(false)
    , parts()
  {}

  eolian_type_instance(std::initializer_list<eolian_type> il,
                       bool is_out_ = false,
                       bool is_nonull_ = false)
    : is_out(is_out_)
    , is_nonull(is_nonull_)
    , parts(il)
  {}

  explicit eolian_type_instance(std::size_t size)
    : is_out(false)
    , is_nonull(false)
    , parts(size)
  {}

  bool empty() const { return parts.empty(); }
  std::size_t size() const { return parts.size(); }

  eolian_type& front() { return parts.front(); }
  eolian_type const& front() const { return parts.front(); }

  bool is_out;
  bool is_nonull;
  eolian_type_container parts;
};

const efl::eolian::eolian_type
void_type { "void", efl::eolian::eolian_type::simple_, false, false, false, "", {} };

inline bool
type_is_void(eolian_type_instance const& type)
{
   return type.empty() || type.front().native.compare("void") == 0;
}

inline bool
type_is_binding(eolian_type const& type)
{
   return !type.binding.empty();
}

inline bool
type_is_binding(eolian_type_instance const& type)
{
   assert(!type.empty());
   return type_is_binding(type.front());
}

inline bool
type_is_out(eolian_type_instance const& type)
{
   return type.is_out;
}

inline bool
type_is_class(eolian_type const& type)
{
   return type.is_class;
}

inline bool
type_is_class(eolian_type_instance const& type)
{
   assert(!type.empty());
   return type_is_class(type.front());
}

inline bool
type_is_nonull(eolian_type_instance const& type)
{
   return type.is_nonull;
}

inline eolian_type
type_to_native(eolian_type const& type)
{
   eolian_type native(type);
   native.binding.clear();
   native.category = eolian_type::simple_;
   native.is_class = false;
   return native;
}

inline eolian_type
type_to_native(eolian_type_instance const& type_ins)
{
   assert(!type_ins.empty());
   return type_to_native(type_ins.front());
}

inline std::string
type_to_native_str(eolian_type_instance const& type_ins)
{
   return type_to_native(type_ins).native;
}

inline bool
type_is_complex(eolian_type const& type)
{
   return type.category == eolian_type::complex_;
}

inline bool
type_is_complex(eolian_type_instance const& type_ins)
{
   assert(!type_ins.empty());
   return type_is_complex(type_ins.front());
}

template <typename T>
inline bool
type_is_callback(T const&);

template <>
inline bool
type_is_callback(eolian_type const& type)
{
   return type.category == eolian_type::callback_;
}

template <>
inline bool
type_is_callback(eolian_type_instance const& type_ins)
{
   return type_is_callback(type_ins.front());
}

struct eo_generator_options
{
   includes_container_type cxx_headers;
   includes_container_type c_headers;
};

struct eo_class
{
   enum eo_class_type
     {
       regular_, regular_noninst_, interface_, mixin_
     };

   eo_class_type type;
   std::string name;
   std::string eo_name;
   ancestors_container_type parents;
   ancestors_container_type ancestors;
   constructors_container_type constructors;
   functions_container_type functions;
   events_container_type events;
   std::string comment;
   std::string name_space;
};

struct eo_parameter
{
   eolian_type_instance type;
   std::string name;
};

struct eo_constructor
{
   std::string name;
   parameters_container_type params;
   std::string comment;
};

struct eo_function
{
   enum eo_function_type
     {
       regular_, class_
     };
   eo_function_type type;
   std::string name;
   std::string impl;
   eolian_type_instance ret;
   parameters_container_type params;
   std::string comment;
};

struct eo_event
{
   std::string name;
   std::string eo_name;
   //parameters_container_type params; // XXX desirable.
   std::string comment;
};


inline bool
function_is_void(eo_function const& func)
{
   return func.ret.empty() || func.ret.front().native.compare("void") == 0;
}

inline bool
function_is_static(eo_function const& func)
{
   return func.type == eo_function::class_;
}

inline unsigned int
parameters_count_callbacks(parameters_container_type const& parameters)
{
   unsigned int r = 0u;
   for (auto first = parameters.begin(), last = parameters.end()
          ; first != last ; ++first)
     if(type_is_callback(first->type) && first + 1 != last)
       ++r;
   return r;
}

inline parameters_container_type::const_iterator
parameters_find_callback(parameters_container_type const& parameters)
{
   for (auto it = parameters.cbegin(), last = parameters.cend();
        it != last; ++it)
     {
        if (type_is_callback((*it).type) && it + 1 != last)
          return it;
     }
   return parameters.cend();
}

} } // namespace efl { namespace eolian {

#endif // EFL_EOLIAN_CXX_EO_TYPES_HH
