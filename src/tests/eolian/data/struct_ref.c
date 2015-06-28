#ifndef _TYPES_OUTPUT_C_
#define _TYPES_OUTPUT_C_

#ifndef _STRUCT_EO_CLASS_TYPE
#define _STRUCT_EO_CLASS_TYPE

typedef Eo Struct;

#endif

#ifndef _STRUCT_EO_TYPES
#define _STRUCT_EO_TYPES

typedef struct _Foo Foo;

typedef struct {
  Foo a;
  struct _Foo b;
} Bar;

struct Named {
  int field;
  const char *something;
};

struct Another {
  struct Named field;
};

struct _Foo {
  int field;
  float another;
};

struct Opaque;


#endif
#define STRUCT_CLASS struct_class_get()

EAPI const Eo_Class *struct_class_get(void) EINA_CONST;

/**
 *
 * No description supplied.
 *
 * @param[in] idx No description supplied.
 *
 */
EOAPI char * struct_foo(int idx);


#endif
