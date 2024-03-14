#ifndef ARNELCOMPILER
#define ARNELCOMPILER

#include "parser.h"
#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define imm_print(comp, imm, is_imm) do if (is_imm) foutputf(comp, "w %zu", imm); else foutputf(comp, "%%x%zu", imm); while (0)

typedef struct local {
    size_t arg;
} local_t;

typedef enum qbe_type {
    T_W, T_L, T_S, T_D, T_DATA, T_RET
} qbe_type_t;

static const char type_strings[] = {[T_DATA] = 'l',  [T_W] = 'w', [T_L] = 'l', [T_S] = 's', [T_D] = 'd' };

typedef struct val {
    qbe_type_t t;
    uint8_t imm;
    uintptr_t val;
} val_t;

#define as_w(v) ((val_t){T_W,0,v})
#define as_l(v) ((val_t){T_L,0,v})
#define as_s(v) ((val_t){T_S,0,v})
#define as_d(v) ((val_t){T_D,0,v})
#define ret ((val_t){T_RET,1,0})
#define as_data(v) ((val_t){T_DATA,1,v})

#define p_typed(str) type_strings[(str).t], (str).val

//INFO: the lexical scope of the compiler at the moment.
// it provides an interface to know if a symbol exists now and can be used;
struct scope {
    unordered_map_t types;
    unordered_map_t symbols;
    unordered_map_t modules;
};

enum { SCOPED_TYPE, SCOPED_SYMBOL, SCOPED_MODULE };

typedef struct scoped {
    uintptr_t thing;
    uint8_t type;
} scoped_t;

uintptr_t scope_get_scoped(struct scope *scope, view_t name, uint8_t type);
struct scope *scope_put_scoped(struct scope *scope, view_t name, uintptr_t item, uint8_t type);

#define set_scope_type(comp, name, item) scope_put_scoped(&(comp)->scope, (name), (uintptr_t)(item), SCOPED_TYPE)
#define set_scope_symbol(comp, item) scope_put_scoped(&(comp)->scope, (item)->symbol_name, (uintptr_t)(item), SCOPED_SYMBOL)
#define set_scope_module(comp, name, item) scope_put_scoped(&(comp)->scope, (name), (uintptr_t)(item), SCOPED_MODULE)
#define get_scope_type(comp, name) (typeid_t)scope_get_scoped(&(comp)->scope, (name), SCOPED_TYPE)
#define get_scope_symbol(comp, name) (symbol_t*)scope_get_scoped(&(comp)->scope, (name), SCOPED_SYMBOL)
#define get_scope_module(comp, name) (symbol_t*)scope_get_scoped(&(comp)->scope, (name), SCOPED_MODULE)

#define alias_symbol(comp, sym, alias) scope_put_scoped(&(comp)->scope, view_from_literal(alias), (uintptr_t)(sym), SCOPED_SYMBOL)
#define alias_symbol_v(comp, sym, view) scope_put_scoped(&(comp)->scope, view, (uintptr_t)(sym), SCOPED_SYMBOL)

struct compiler_t {
    FILE *output_file;

    struct scope scope;

    size_t temps_stack;
    uint32_t ids;

    unordered_map_t *locals;
    
};

#define foutputf(comp, ...) fprintf(comp->output_file, __VA_ARGS__)

struct compiler_t *init_compiler(struct compiler_t * comp);
void comp_set_output(struct compiler_t * comp, FILE *f);
void comp_file(struct compiler_t *comp, const char *in_file);
void comp_from_string(struct compiler_t *comp, const char *program, size_t program_size);

#endif
