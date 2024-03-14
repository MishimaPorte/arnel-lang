#ifndef ARNELTYPES
#define ARNELTYPES

#include "bytes.h"
#include <stddef.h>
#include <stdint.h>

typedef size_t typeid_t;

typedef struct type {
    view_t typename;

    typeid_t __type_id;

    //INFO: place for concrete type-related info:
    // like function argument types etc
    void *type_info;
} type_t;

// typedef struct field_type {
//     typeid_t type;
//     strview_t name;
//     size_t size;
//     size_t alignment;
// } field_t;
//
// typedef struct struct_type {
//     void *methods;
//     typeid_t fields[];
// } struct_t;
//
// typedef struct array_type {
//     typeid_t arr_of;
//     size_t number;
// } arr_t;
//
// typedef struct ptr_type {
//     typeid_t ptr_to;
// } ptr_t;

typedef struct function_signature {
    typeid_t return_type_id;
    size_t arity;
    typeid_t arg_types[];
} sig_t;

//INFO: returned pointer is considered valid only
//until the next type is allocated since they are allocated in a dynamic array;
//use get_type to get a type by its id instead
type_t *alloc_type(view_t name);
type_t *alloc_type_from_cstr(const char *name);

type_t *get_type(typeid_t type_id);
type_t *get_type_for_name(view_t name);

void init_typesystem(void);
enum __builtin_types {
    __BAD_TYPE = (typeid_t)0,
    TYPE_FLOAT = (typeid_t)1,
    TYPE_STRING = (typeid_t)2,
    TYPE_CSTRING = (typeid_t)3,
    TYPE_BOOL = (typeid_t)4,
    TYPE_NOTHING = (typeid_t)5,
    TYPE_VOID = (typeid_t)6,
    TYPE_MODULE = (typeid_t)7,
    TYPE_INTEGER = (typeid_t)8,
    TYPE_DOUBLE = (typeid_t)9,
    TYPE_TYPE = (typeid_t)10,
    TYPE_VARIADIC = (typeid_t)11,
};

typedef struct types {
    size_t len, cap;
    typeid_t *items;
} types_t;

typeid_t pop_type(types_t *s);
typeid_t peek_type(types_t *s);
void put_type(types_t *s, typeid_t n);

#endif

