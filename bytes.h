#ifndef __BYTES
#define __BYTES
#include <sys/types.h>
#include <inttypes.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>
#include <wchar.h>

typedef struct __sized {
    const char *chars;
    size_t len;
} view_t;

#define view_print(v) (v).len, (v).chars
#define view_ptr_print(v) (v)->len, (v)->chars
#define view_from_literal(cstr) ((view_t){.chars = cstr, .len = sizeof(cstr) - 1})

typedef struct __buffer {
    size_t len;
    size_t cap;
    char *buf;
} buf_t;

view_t view_make(const char *str, size_t len);
view_t *view_slice(view_t *in, view_t *out, size_t start, size_t end);
char *view_cstr(view_t view);
int view_cmp(view_t view, view_t view_2);
view_t view_trim_left(view_t view);
view_t view_chop_predicate(view_t *view, int (*predicate)(char c));
view_t view_chopn(view_t *view, size_t n);

#ifdef __BYTES_IMPLEMENTATION
view_t view_chopn(view_t *view, size_t n)
{
    if (view->len < n) return (view_t) {0};
    view_t s = (view_t) {
        .chars = view->chars,
        .len = n,
    };

    view->chars += n;
    view->len -= n;

    return s;
};

view_t view_chop_predicate(view_t *view, int (*predicate)(char c))
{
    size_t end = 0;
    while (predicate(*(view->chars + end))) end++;

    view_t ret =  (view_t) {
        .chars = view->chars,
        .len = end,
    };
    view->chars += end;
    view->len -= end;
    return ret;
};

view_t view_trim_left(view_t view)
{
    while (isspace(*view.chars)) view.chars++;
    return view;
};

int view_cmp(view_t view, view_t view_2)
{
    if (view.len != view_2.len) return view.len - view_2.len;
    return memcmp(view.chars, view_2.chars, view.len);
};

view_t view_make(const char *str, size_t len) { return (view_t){.len = len, .chars = str};}

view_t *view_slice(view_t *in, view_t *out, size_t start, size_t end)
{
    if (start >= in->len || end >= in->len || end <= start) return NULL;

    out->len = end - start;
    out->chars = &in->chars[start];

    return out;
};

char *view_cstr(view_t view)
{
    char *new_cstr = malloc(view.len+1);
    if (new_cstr == NULL) return NULL;

    memcpy(new_cstr, view.chars, view.len);
    new_cstr[view.len] = '\0';
    return new_cstr;
};
#endif

buf_t *alloc_buf(size_t initial_cap);

void *buf_alloc_to(buf_t *buf, size_t size);

buf_t *buf_write_byte(buf_t *buf, uint8_t b);

buf_t *buf_write_short(buf_t *buf, uint16_t sh);
buf_t *buf_write_long(buf_t *buf, uint32_t l);
buf_t *buf_write_longlong(buf_t *buf, uint64_t ll);

buf_t *buf_write_short_be(buf_t *buf, uint16_t sh);
buf_t *buf_write_long_be(buf_t *buf, uint32_t l);
buf_t *buf_write_longlong_be(buf_t *buf, uint64_t ll);

buf_t *buf_write_short_le(buf_t *buf, uint16_t sh);
buf_t *buf_write_long_le(buf_t *buf, uint32_t l);
buf_t *buf_write_longlong_le(buf_t *buf, uint64_t ll);

buf_t *buf_write_cstr(buf_t *buf, const char *cstr);
buf_t *buf_write_sized(buf_t *buf, const char *cstr, size_t size);
buf_t *buf_write_view(buf_t *buf, view_t *view);
buf_t *buf_write_format(buf_t *buf, const char *format, ...);
buf_t *buf_write_file(buf_t *buf, const char *file);
buf_t *buf_trunc(buf_t *buf);
buf_t *buf_chop(buf_t *buf, size_t num);
view_t buf_get_view(buf_t *buf, size_t start, size_t end);

#ifdef __BYTES_IMPLEMENTATION
void *buf_alloc_to(buf_t *buf, size_t size)
{
    if (buf->cap < buf->len + size) {
        buf->buf = realloc(buf->buf, buf->cap + size);
        if (buf->buf == NULL) return NULL; 
        buf->cap += size;
    }

    void *ret = &buf->buf[buf->len];
    buf->len+=size;
    return ret;
};

buf_t *buf_write_byte(buf_t *buf, uint8_t b)
{
    if (buf->cap < buf->len + 1) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    buf->buf[buf->len++] = b;
    return buf;
};

buf_t *buf_write_short(buf_t *buf, uint16_t sh)
{
    if (buf->cap < buf->len + 2) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    *(uint16_t*)&buf->buf[buf->len] = sh;
    buf->len += 2;
    return buf;
};

buf_t *buf_write_long(buf_t *buf, uint32_t l) {
    if (buf->cap < buf->len + 4) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    *(uint32_t*)&buf->buf[buf->len] = l;
    buf->len += 4;
    return buf;
};

buf_t *buf_write_longlong(buf_t *buf, uint64_t ll) {
    if (buf->cap < buf->len + 8) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    *(uint64_t*)&buf->buf[buf->len] = ll;
    buf->len += 8;
    return buf;
};

buf_t *buf_write_short_le(buf_t *buf, uint16_t val)
{
    if (buf->cap < buf->len + 2) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    return buf;
};

buf_t *buf_write_long_le(buf_t *buf, uint32_t val) {
    if (buf->cap < buf->len + 4) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*2)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*3)) & 0xff;
    return buf;
};

buf_t *buf_write_longlong_le(buf_t *buf, uint64_t val) {
    if (buf->cap < buf->len + 8) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*2)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*3)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*4)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*5)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*6)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*7)) & 0xff;
    return buf;
};

buf_t *buf_write_short_be(buf_t *buf, uint16_t val)
{
    if (buf->cap < buf->len + 2) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    return buf;
};

buf_t *buf_write_long_be(buf_t *buf, uint32_t val) {
    if (buf->cap < buf->len + 4) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    buf->buf[buf->len++] = (val >> (8*3)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*2)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    return buf;
};

buf_t *buf_write_longlong_be(buf_t *buf, uint64_t val) {
    if (buf->cap < buf->len + 8) {
        buf->buf = realloc(buf->buf, buf->cap + 10);
        buf->cap += 10;
        if (buf->buf == NULL) return NULL; 
    }

    buf->buf[buf->len++] = (val >> (8*7)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*6)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*5)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*4)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*3)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*2)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*1)) & 0xff;
    buf->buf[buf->len++] = (val >> (8*0)) & 0xff;
    return buf;
};

buf_t *buf_write_file(buf_t *buf, const char *file)

{
    FILE *fin = fopen(file, "rb");
    if (fin == NULL) return NULL;;
    fseek(fin, 0l, SEEK_END);
    size_t fsize = ftell(fin);
    rewind(fin);
    if (buf->cap <= buf->len + fsize) {
        buf->buf = realloc(buf->buf, buf->cap + fsize);
        if (buf->buf == NULL) return NULL; 
        buf->cap += fsize;
    }

    size_t i = fread(&buf->buf[buf->len], sizeof(char), fsize, fin);
    buf->len += fsize;
    if (i != fsize) return NULL;
    fclose(fin);

    return buf;
};

view_t buf_get_view(buf_t *buf, size_t start, size_t end)
{
    if (buf->len < end) return (view_t) {
        .len = 0,
        .chars = NULL,
    }; else if (buf->len < start) return (view_t) {
        .len = 0,
        .chars = NULL,
    }; else if (end <= start) return (view_t) {
        .len = 0,
        .chars = NULL,
    };

    return (view_t) {
        .chars = buf->buf + start,
        .len = end - start,
    };
};

buf_t *buf_chop(buf_t *buf, size_t num)
{
    if (buf->len <= num) buf->len = 0;
    else buf->len = buf->len - num;

    return buf;
};

buf_t *alloc_buf(size_t initial_cap)
{
    buf_t *buf = malloc(sizeof(buf_t));
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

buf_t *buf_write_cstr(buf_t *buf, const char *cstr)
{
    size_t cstr_len = strlen(cstr);

    if (buf->cap <= buf->len + cstr_len) {
        buf->buf = realloc(buf->buf, buf->cap + cstr_len);
        buf->cap += cstr_len;
        if (buf->buf == NULL) return NULL; 
    }

    strcpy(&buf->buf[buf->len], cstr);
    buf->len += cstr_len;

    return buf;
};

buf_t *buf_write_sized(buf_t *buf, const char *cstr, size_t size)
{
    if (buf->cap <= buf->len + size) {
        buf->buf = realloc(buf->buf, buf->cap + size);
        buf->cap += size;
        if (buf->buf == NULL) return NULL; 
    }

    memcpy(&buf->buf[buf->len], cstr, size);
    buf->len += size;

    return buf;
};

buf_t *buf_write_view(buf_t *buf, view_t *view)
{
    if (buf->cap <= buf->len + view->len) {
        buf->buf = realloc(buf->buf, buf->cap + view->len);
        buf->cap += view->len;
        if (buf->buf == NULL) return NULL; 
    }


    memcpy(&buf->buf[buf->len], view->chars, view->len);
    buf->len += view->len;

    return buf;
};

buf_t *buf_trunc(buf_t *buf)
{
    buf->len = 0;
    return buf;
};

buf_t *buf_write_format(buf_t *buf, const char *format, ...)
{

    va_list args;
    va_start(args, format);
    //determining the size of formatted output to make the buffer large enough
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (buf->cap <= buf->len + size) {
        buf->buf = realloc(buf->buf, buf->cap + size);
        buf->cap += size;
        if (buf->buf == NULL) return NULL; 
    }

    va_start(args, format);
    vsprintf(&buf->buf[buf->len], format, args);
    va_end(args);

    buf->len += size;

    return buf;
};

#endif

#endif
