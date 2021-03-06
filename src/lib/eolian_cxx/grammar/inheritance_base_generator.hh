
#ifndef EOLIAN_CXX_STD_INHERITANCE_GENERATOR_HH
#define EOLIAN_CXX_STD_INHERITANCE_GENERATOR_HH

#include <iosfwd>
#include <cassert>
#include <algorithm>

#include "eo_types.hh"
#include "tab.hh"
#include "parameters_generator.hh"
#include "eo_class_functions_generator.hh"

namespace efl { namespace eolian { namespace grammar {

inline std::string
_ns_as_prefix(eo_class const& cls)
{
   // XXX Use eolian_cxx::find_replace() instead.
   std::string s = cls.name_space;
   std::string::size_type found = s.find("::");
   while (found != std::string::npos)
     {
        s.replace(found, 2, "_");
        found = s.find("::");
     }
   return s;
}

struct inheritance_operation
{
   eo_class const& _cls;
   eo_function const& _func;
   functions_container_type::size_type _idx;
   inheritance_operation(eo_class const& cls, eo_function const& func, functions_container_type::size_type idx)
     : _cls(cls), _func(func), _idx(idx)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_operation const& x)
{
   assert(x._idx < x._cls.functions.size());
   eo_function const& func = x._func;
   out << tab(1)
       << "ops[" << x._idx << "].func = reinterpret_cast<void*>(& ::"
       << _ns_as_prefix(x._cls) << "_"
       << x._cls.name << "_" << func.name << "_wrapper<T>);" << endl
       << tab(1) << "ops[" << x._idx << "].api_func = reinterpret_cast<void*>(& ::"
       << func.impl << ");" << endl
       << tab(1) << "ops[" << x._idx << "].op = EO_OP_OVERRIDE;" << endl
       << tab(1) << "ops[" << x._idx << "].op_type = EO_OP_TYPE_REGULAR;" << endl // XXX class ops
       << tab(1) << "ops[" << x._idx << "].doc = NULL;" << endl
       << endl;
   return out;
}

struct inheritance_operations_description
{
   eo_class const& _cls;
   inheritance_operations_description(eo_class const& cls)
     : _cls(cls)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_operations_description const& x)
{
   std::string s;

   out << "template <typename T>"
       << endl << "int initialize_operation_description(::efl::eo::detail::tag<"
       << full_name(x._cls) << ">" << endl
       << tab(11)
       << ", Eo_Op_Description* ops)" << endl
       << "{" << endl
       << tab(1) << "(void)ops;" << endl;

   auto funcs = x._cls.functions;
   auto part = std::stable_partition(funcs.begin(), funcs.end(), [](eo_function const& f){ return f.scope == eolian_scope::public_; });

   functions_container_type::size_type op_idx = 0;
   for (auto it = funcs.begin(); it != part; ++it, ++op_idx)
     {
        out << inheritance_operation(x._cls, *it, op_idx);
     }

   if (part != funcs.end())
     {
        out << scope_guard_head(x._cls, *part);
        for (auto it = part; it != funcs.end(); ++it, ++op_idx)
          out << inheritance_operation(x._cls, *it, op_idx);
        out << scope_guard_tail(x._cls, *part);
     }

   for (std::string const& parent : x._cls.parents)
     {
        out << tab(1)
            << "initialize_operation_description<T>(::efl::eo::detail::tag<::"
            << parent << ">(), &ops[operation_description_class_size< "
            << full_name(x._cls) << " >::value" << s << "]);" << endl;

        s += " + operation_description_class_size<::" + parent + ">::value";
     }

   out << tab(1) << "return 0;" << endl
       << "}" << endl;

   return out;
}

struct inheritance_wrappers
{
   eo_class const& _cls;
   inheritance_wrappers(eo_class const& cls) : _cls(cls) {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_wrappers const& x)
{
   functions_container_type::const_iterator it,
     first = x._cls.functions.begin(),
     last = x._cls.functions.end();
   for (it = first; it != last; ++it)
     {
        eo_function const& func = *it;

        out << scope_guard_head(x._cls, func);

        out << "template <typename T>" << endl
            << reinterpret_type(func.ret) << " "
            << _ns_as_prefix(x._cls) << "_"
            << x._cls.name << "_" << func.name
            << "_wrapper(Eo* objid EINA_UNUSED, "
            << "::efl::eo::detail::Inherit_Private_Data* self"
            << (func.params.size() ? ", " : "")
            << parameters_c_declaration(func.params)
            << ")" << endl
            << "{" << endl;

        if (!function_is_void(func))
          out << tab(1) << reinterpret_type(func.ret) << " _tmp_ret{};" << endl;

        out << tab(1)
            << "try" << endl
            << tab(2) << "{" << endl
            << tab(3)
            << (!function_is_void(func) ? "_tmp_ret = ": "")
            << "static_cast<T*>(self->this_)->"
            << func.name << "(" << parameters_cxx_list(func.params) << ");" << endl
            << tab(2) << "}" << endl
            << tab(1) << "catch (...)" << endl
            << tab(2) << "{" << endl
            << tab(3) << "eina_error_set( ::efl::eina::unknown_error() );" << endl
            << tab(2) << "}" << endl;

        if (!function_is_void(func))
          out << tab(1) << "return _tmp_ret;" << endl;

        out << "}" << endl;

        out << scope_guard_tail(x._cls, func) << endl;
     }
   return out;
}

struct inheritance_base_operations_size
{
   eo_class const& _cls;
   functions_container_type const& _funcs;
   inheritance_base_operations_size(eo_class const& cls, functions_container_type const& funcs)
     : _cls(cls), _funcs(funcs)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_base_operations_size const& x)
{
   out << "template<>"
       << endl << "struct operation_description_class_size< "
       << full_name(x._cls) << " >" << endl
       << "{" << endl
       << tab(1) << "static constexpr int value = "
       << x._funcs.size();

   for (std::string const& parent : x._cls.parents)
     {
        out << " + operation_description_class_size<::" << parent << " >::value";
     }

   out << ";" << endl
       << "};" << endl
       << endl;

   return out;
}

struct inheritance_base_operations_size_scopes
{
   eo_class const& _cls;
   inheritance_base_operations_size_scopes(eo_class const& cls)
     : _cls(cls)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_base_operations_size_scopes const& x)
{
   auto funcs = x._cls.functions;
   auto part = std::stable_partition(funcs.begin(), funcs.end(), [](eo_function const& f){ return f.scope == eolian_scope::public_; });

   out << "#ifdef " << name_upper(x._cls) << "_PROTECTED" << endl
       << inheritance_base_operations_size(x._cls, funcs)
       << "#else" << endl
       << inheritance_base_operations_size(x._cls, {funcs.begin(), part})
       << "#endif" << endl << endl;
   return out;
}

struct inheritance_base_operations_extensions
{
   eo_class const& _cls;
   inheritance_base_operations_extensions(eo_class const& cls)
     : _cls(cls)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_base_operations_extensions const& x)
{
   eo_class const& cls = x._cls;
   ancestors_container_type::const_iterator it, first = cls.parents.begin();
   ancestors_container_type::const_iterator last = cls.parents.end();

   for (it = first; it != last; ++it)
     {
        out << endl
            << tab(3) << (it == first ? ": " : ", ")
            << "virtual operations< ::" << *it
            << " >::template type<T>";
     }

   return out << endl;
}

struct inheritance_base_operations_function
{
   eo_class const& _cls;
   eo_function const& _func;
   inheritance_base_operations_function(eo_class const& cls, eo_function const& func)
     : _cls(cls) , _func(func)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_base_operations_function const& x)
{
   eo_function const& func = x._func;
   bool is_void = function_is_void(func);

   if (parameters_count_callbacks(func.params))
     out << template_parameters_declaration(func.params, 2) << tab(2);
   else
     out << tab(2) << "virtual ";

   out << reinterpret_type(func.ret) << " "
       << func.name << "("
       << parameters_declaration(func.params) << ")" << endl
       << tab(2) << "{" << endl;

   if (!is_void)
     out << tab(3) << func.ret.front().native << " _tmp_ret = {};"  << endl;

   out << callbacks_heap_alloc("dynamic_cast<T*>(this)->_eo_ptr()", func.params, function_is_static(x._func), 3)
       << endl;

   out << tab(3)
       << "eo_do_super(dynamic_cast<T*>(this)->_eo_ptr()," << endl
       << tab(5) << "dynamic_cast<T*>(this)->_eo_class()," << endl
       << tab(5) << function_call(func) << ");" << endl;

   if (!is_void)
     out << tab(4) << "return " << to_cxx(func.ret, "_tmp_ret") << ";" << endl;

   return out << tab(2) << "}" << endl;
}

struct inheritance_base_operations
{
   eo_class const& _cls;
   inheritance_base_operations(eo_class const& cls) : _cls(cls) {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_base_operations const& x)
{
   out << "template<>" << endl
       << "struct operations< "
       << full_name(x._cls) << " >" << endl
       << "{" << endl
       << tab(1) << "template <typename T>" << endl
       << tab(1) << "struct type" << inheritance_base_operations_extensions(x._cls)
       << tab(1) << "{" << endl;
   functions_container_type::const_iterator it,
     first = x._cls.functions.begin(),
     last = x._cls.functions.end();
   for (it = first; it != last; ++it)
     {
        out << scope_guard_head(x._cls, *it);
        out << inheritance_base_operations_function(x._cls, *it);
        out << scope_guard_tail(x._cls, *it) << endl;
     }
   out << tab(1) << "};" << endl
       << "};" << endl << endl;
   return out;
}

struct inheritance_eo_class_getter
{
   eo_class const& _cls;
   inheritance_eo_class_getter(eo_class const& cls)
     : _cls(cls)
   {}
};

inline std::ostream&
operator<<(std::ostream& out, inheritance_eo_class_getter const& x)
{
   out << "inline Eo_Class const* get_eo_class(tag<"
       << full_name(x._cls) << ">)" << endl
       << "{" << endl
       << tab(1) << "return (" << x._cls.eo_name << ");" << endl
       << "}" << endl << endl;
   return out;
}

inline void
eo_inheritance_detail_generator(std::ostream& out, eo_class const& cls)
{
   if(cls.eo_name != "EO_BASE_CLASS")
     out << inheritance_wrappers(cls)
         << "namespace efl { namespace eo { namespace detail {" << endl << endl
         << inheritance_base_operations(cls) << endl
         << inheritance_base_operations_size_scopes(cls)
         << inheritance_operations_description(cls)
         << inheritance_eo_class_getter(cls)
         <<  "} } }" << endl;
}

} } } // namespace efl { namespace eolian { namespace grammar {

#endif // EOLIAN_CXX_STD_INHERITANCE_GENERATOR_HH
