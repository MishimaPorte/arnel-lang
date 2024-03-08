#ifndef DA
#define DA
#include <stddef.h>

#define dynamic_array_t(t, name) name {                           \
    t *items;                                                     \
    size_t cap, len;                                              \
};                                                                \
struct name *name##_append(struct name *da, t item)               \
{                                                                 \
    if (da->len == da->cap) {                                     \
        da->items = realloc(da->items, da->cap * 2 *sizeof(t));   \
        if (da->items == NULL) return NULL;                       \
        da->cap *= 2;                                             \
    };                                                            \
    da->items[da->len++] = item;                                  \
    return da;                                                    \
};                                                                \
t name##_get(struct name *da, size_t i)                           \
{                                                                 \
    if (i < da->len) return da->items[i];                         \
    return (t){0};                                                \
};                                                                \
struct name *name##_init(struct name *da, size_t i)               \
{                                                                 \
    if ((da->items = malloc(i * sizeof(t))) == NULL) return NULL; \
    da->cap = i;                                                  \
    da->len = 0;                                                  \
    return da;                                                    \
};                                                                \
struct name *name##_remove(struct name *da, size_t i, t *out)     \
{                                                                 \
    if (i >= da->len) return NULL;                                \
    if (out != NULL) *out = da->items[i];                         \
    for (size_t x = da->len--; i < x; i++) {                      \
        da->items[i] = da->items[i + 1];                          \
    };                                                            \
    return da;                                                    \
};                                                                \

#endif
