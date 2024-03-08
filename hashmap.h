#ifndef HASHMAP
#define HASHMAP
#include <stddef.h>

#ifndef INITIAL_HM_CAP
#define INITIAL_HM_CAP 12
#endif
#ifndef HM_FREE_RATIO
#define HM_FREE_RATIO 0.7
#endif

typedef size_t (*hash_func_t)(const char*, size_t);
typedef struct {const char *key; size_t size; void *val;} kval_t;
typedef struct {
    size_t cap, len;
    kval_t *values;
    hash_func_t hash;
} unordered_map_t;


unordered_map_t *um_init(unordered_map_t *m);
void um_set_hash_function(unordered_map_t *m, hash_func_t);
void um_uninit(unordered_map_t *m);
void um_set(unordered_map_t *m, const char *key, size_t ksize, void *val);
void *um_get(unordered_map_t *m, const char* key, size_t ksize);

void um_set_cstr(unordered_map_t *m, const char *cstr, void *val);
void *um_get_cstr(unordered_map_t *m, const char* cstr);

#ifdef HASHMAP_IMPLEMENTATION
static size_t __default_naive_hash(const char *key, size_t size)
{
    size_t ret = 0;
    for (size_t i = 0; i < size; i++)
        ret += key[i];

    return ret;
};

unordered_map_t *um_init(unordered_map_t *m)
{
    m->values = calloc(sizeof(kval_t), INITIAL_HM_CAP);
    if (m->values == NULL) return NULL;
    m->cap = INITIAL_HM_CAP;
    m->len = 0;
    m->hash = &__default_naive_hash;
    return m;
};

void um_set_hash_function(unordered_map_t *m, hash_func_t hash)
{
    m->hash = hash;
};

void um_uninit(unordered_map_t *m)
{
    m->values = realloc(m->values, 0);
    m->cap = 0;
    m->len = 0;
};

void __rehash(unordered_map_t *m)
{
    kval_t *old = m->values;
    size_t old_cap = m->cap;

    m->values = calloc(m->cap*=2, sizeof(kval_t));
    m->len = 0;
    for (size_t i = 0; i < old_cap; i++)
        if (old[i].key != NULL) {
            um_set(m, old[i].key, old[i].size, old[i].val);
        }
    free(old);
};

void um_set(unordered_map_t *m, const char *key, size_t ksize, void *val)
{
    if (((float)m->len/(float)m->cap) > HM_FREE_RATIO) __rehash(m);
    size_t hashed = m->hash(key, ksize) % m->cap;
    while (1) {
        if (ksize == m->values[hashed].size && memcmp(key, m->values[hashed].key, ksize) == 0) {
            m->values[hashed].val = val;
            return;
        } else if (m->values[hashed].key == NULL) {
            m->len++;
            m->values[hashed].key = key;
            m->values[hashed].val = val;
            m->values[hashed].size = ksize;
            return;
        } else hashed = (hashed+1) % m->cap;
    }
};



void *um_get(unordered_map_t *m, const char* key, size_t ksize)
{
    size_t hashed = m->hash(key, ksize) % m->cap;
    while (1) {
        if (ksize == m->values[hashed].size && memcmp(key, m->values[hashed].key, ksize) == 0) return m->values[hashed].val;
        else if (m->values[hashed].key == NULL) return NULL;
        else hashed = (hashed+1) % m->cap;
    }
    return NULL;
};

void um_set_cstr(unordered_map_t *m, const char *cstr, void *val);
void *um_get_cstr(unordered_map_t *m, const char* cstr);
#endif

#endif
