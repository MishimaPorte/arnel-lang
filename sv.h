#ifndef __STRINGS
#define __STRINGS
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define __STRINGS_IMPLEMENTATION

#include <stddef.h>
#include <wchar.h>

typedef struct __sized_string {
    const char *chars;
    size_t len;
} strview_t;

#define sv_print(sv) (sv).len, (sv).chars
#define sv_ptr_print(sv) (sv)->len, (sv)->chars

typedef struct __string_buffer {
    size_t len;
    size_t cap;
    char *buf;
} strbuf_t;

strview_t sv_make(const char *str, size_t len);
strview_t *sv_slice(strview_t *in, strview_t *out, size_t start, size_t end);
char *sv_cstr(strview_t sv);
int sv_cmp(strview_t sv, strview_t sv_2);
strview_t sv_trim_left(strview_t sv);
strview_t sv_chop_predicate(strview_t *sv, int (*predicate)(char c));
strview_t sv_chopn(strview_t *sv, size_t n);

#ifdef __STRINGS_IMPLEMENTATION
strview_t sv_chopn(strview_t *sv, size_t n)
{
    if (sv->len < n) return (strview_t) {0};
    strview_t s = (strview_t) {
        .chars = sv->chars,
        .len = n,
    };

    sv->chars += n;
    sv->len -= n;

    return s;
};

strview_t sv_chop_predicate(strview_t *sv, int (*predicate)(char c))
{
    size_t end = 0;
    while (predicate(*(sv->chars + end))) end++;

    strview_t ret =  (strview_t) {
        .chars = sv->chars,
        .len = end,
    };
    sv->chars += end;
    sv->len -= end;
    return ret;
};

strview_t sv_trim_left(strview_t sv)
{
    while (isspace(*sv.chars)) sv.chars++;
    return sv;
};

int sv_cmp(strview_t sv, strview_t sv_2)
{
    if (sv.len != sv_2.len) return sv.len - sv_2.len;
    return memcmp(sv.chars, sv_2.chars, sv.len);
};

strview_t sv_make(const char *str, size_t len) { return (strview_t){.len = len, .chars = str};}

strview_t *sv_slice(strview_t *in, strview_t *out, size_t start, size_t end)
{
    if (start >= in->len || end >= in->len || end <= start) return NULL;

    out->len = end - start;
    out->chars = &in->chars[start];

    return out;
};

char *sv_cstr(strview_t sv)
{
    char *new_cstr = malloc(sv.len+1);
    if (new_cstr == NULL) return NULL;

    memcpy(new_cstr, sv.chars, sv.len);
    new_cstr[sv.len] = '\0';
    return new_cstr;
};
#endif

strbuf_t *alloc_strbuf(size_t initial_cap);

strbuf_t *strbuf_append_cstr(strbuf_t *buf, const char *cstr);
strbuf_t *strbuf_append_sized(strbuf_t *buf, const char *cstr, size_t size);
strbuf_t *strbuf_append_sv(strbuf_t *buf, strview_t *sv);
strbuf_t *strbuf_append_format(strbuf_t *buf, const char *format, ...);
strbuf_t *strbuf_append_file(strbuf_t *buf, const char *file);
strbuf_t *strbuf_trunc(strbuf_t *buf);
strbuf_t *strbuf_chop(strbuf_t *buf, size_t num);
strview_t strbuf_get_sv(strbuf_t *buf, size_t start, size_t end);

#ifdef __STRINGS_IMPLEMENTATION
strbuf_t *strbuf_append_file(strbuf_t *buf, const char *file)
{
    FILE *fin = fopen(file, "rb");
    if (fin == NULL) return NULL;;
    fseek(fin, 0l, SEEK_END);
    size_t fsize = ftell(fin);
    rewind(fin);
    if (buf->cap <= buf->len + fsize)
        buf->buf = realloc(buf->buf, buf->cap + fsize);
    if (buf->buf == NULL) return NULL; 

    size_t i = fread(&buf->buf[buf->len], sizeof(char), fsize, fin);
    buf->len += fsize;
    if (i != fsize) return NULL;
    fclose(fin);

    return buf;
};

strview_t strbuf_get_sv(strbuf_t *buf, size_t start, size_t end)
{
    if (buf->len < end) return (strview_t) {
        .len = 0,
        .chars = NULL,
    }; else if (buf->len < start) return (strview_t) {
        .len = 0,
        .chars = NULL,
    }; else if (end <= start) return (strview_t) {
        .len = 0,
        .chars = NULL,
    };

    return (strview_t) {
        .chars = buf->buf + start,
        .len = end - start,
    };
};

strbuf_t *strbuf_chop(strbuf_t *buf, size_t num)
{
    if (buf->len <= num) buf->len = 0;
    else buf->len = buf->len - num;

    return buf;
};

strbuf_t *alloc_strbuf(size_t initial_cap)
{
    strbuf_t *buf = malloc(sizeof(strbuf_t));
    if (buf == NULL) return NULL;
    buf->len = 0;
    buf->cap = initial_cap;
    buf->buf = malloc(initial_cap);
    if (buf->buf == NULL) {
        free(buf);
        return NULL;
    }
    return buf;
};

strbuf_t *strbuf_append_cstr(strbuf_t *buf, const char *cstr)
{
    size_t cstr_len = strlen(cstr);

    if (buf->cap <= buf->len + cstr_len)
        buf->buf = realloc(buf->buf, buf->cap + cstr_len);

    if (buf->buf == NULL) return NULL; 

    strcpy(&buf->buf[buf->len], cstr);
    buf->len += cstr_len;

    return buf;
};

strbuf_t *strbuf_append_sized(strbuf_t *buf, const char *cstr, size_t size)
{
    if (buf->cap <= buf->len + size)
        buf->buf = realloc(buf->buf, buf->cap + size);

    if (buf->buf == NULL) return NULL; 

    memcpy(&buf->buf[buf->len], cstr, size);
    buf->len += size;

    return buf;
};

strbuf_t *strbuf_append_sv(strbuf_t *buf, strview_t *sv)
{
    if (buf->cap <= buf->len + sv->len)
        buf->buf = realloc(buf->buf, buf->cap + sv->len);

    if (buf->buf == NULL) return NULL; 

    memcpy(&buf->buf[buf->len], sv->chars, sv->len);
    buf->len += sv->len;

    return buf;
};

strbuf_t *strbuf_trunc(strbuf_t *buf)
{
    buf->len = 0;
    return buf;
};

strbuf_t *strbuf_append_format(strbuf_t *buf, const char *format, ...)
{

    va_list args;
    va_start(args, format);
    //determining the size of formatted output to make the buffer large enough
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (buf->cap <= buf->len + size)
        buf->buf = realloc(buf->buf, buf->cap + size);
    if (buf->buf == NULL) return NULL; 

    va_start(args, format);
    vsprintf(&buf->buf[buf->len], format, args);
    va_end(args);

    buf->len += size;

    return buf;
};

#endif

#endif
