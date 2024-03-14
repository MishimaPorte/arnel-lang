#include "types.h"
#include "bytes.h"
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static struct {
    size_t __len, __cap;
    type_t *types;
} typesystem;

typeid_t pop_type(types_t *s)
{
    if (s->len == 0)
        return 0;
    return s->items[--s->len];
};

typeid_t peek_type(types_t *s)
{
    if (s->len == 0)
        return 0;
    return s->items[s->len - 1];
};

void put_type(types_t *s, typeid_t n)
{
    if (s->len == s->cap) {
        s->items = realloc(s->items, s->cap*=2);
    }
    s->items[s->len++] = n;
    return;
};


type_t *alloc_type(view_t name)
{
    if (typesystem.__len == typesystem.__cap) {
        typesystem.types = realloc(typesystem.types, typesystem.__cap *= 2);
        if (typesystem.types == NULL) {
            err(69, "could not allocate enougm memory for parsing: %s", strerror(errno));
        }
    }

    typesystem.types[typesystem.__len] = (type_t) {
        .typename = name,
        .__type_id = typesystem.__len,
    };

    return &typesystem.types[typesystem.__len++];
};

type_t *get_type(typeid_t type_id)
{
    if (typesystem.__len <= type_id) return NULL;
    return typesystem.types + type_id;
};

type_t *alloc_type_from_cstr(const char *name)
{
    if (typesystem.__len >= typesystem.__cap) {
        typesystem.types = realloc(typesystem.types, typesystem.__cap *= 2);
        if (typesystem.types == NULL) {
            err(69, "could not allocate enougm memory for parsing: %s", strerror(errno));
        }
    }

    typesystem.types[typesystem.__len] = (type_t) {
        .typename = {
            .chars = name,
            .len = strlen(name),
        },
        .__type_id = typesystem.__len,
    };

    return &typesystem.types[typesystem.__len++];
};

type_t *get_type_for_name(view_t name)
{
    for (size_t i = 0; i < typesystem.__len; i++)
        if (view_cmp(name, typesystem.types[i].typename) == 0)
            return &typesystem.types[i];

    return NULL;
};

void init_typesystem()
{
    typesystem.types = calloc(20, sizeof(type_t));
    typesystem.__cap = 20;
    typesystem.__len = 0;

    //INFO: asserts are use
    assert(__BAD_TYPE == alloc_type_from_cstr("__bad_type")->__type_id);
    assert(TYPE_FLOAT == alloc_type_from_cstr("float")->__type_id);
    assert(TYPE_STRING == alloc_type_from_cstr("str")->__type_id);
    assert(TYPE_CSTRING == alloc_type_from_cstr("cstr")->__type_id);
    assert(TYPE_BOOL == alloc_type_from_cstr("bool")->__type_id);
    assert(TYPE_NOTHING == alloc_type_from_cstr("nothing")->__type_id);
    assert(TYPE_VOID == alloc_type_from_cstr("void")->__type_id);
    assert(TYPE_MODULE == alloc_type_from_cstr("module")->__type_id);
    assert(TYPE_INTEGER == alloc_type_from_cstr("int")->__type_id);
    assert(TYPE_DOUBLE == alloc_type_from_cstr("double")->__type_id);
    assert(TYPE_TYPE == alloc_type_from_cstr("type")->__type_id);
    assert(TYPE_VARIADIC == alloc_type_from_cstr("__variadic")->__type_id);
};

