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
    T_W, T_L, T_S, T_D, T_DATA
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
#define as_data(v) ((val_t){T_DATA,1,v})

#define p_typed(str) type_strings[(str).t], (str).val

struct compiler_t {
    FILE *output_file;

    size_t temps_stack;
    uint8_t is_imm;
    uint32_t ids;


    unordered_map_t *locals;
};

#define foutputf(comp, ...) fprintf(comp->output_file, __VA_ARGS__)

struct compiler_t *init_compiler(struct compiler_t * comp);
void comp_set_output(struct compiler_t * comp, FILE *f);
void comp_file(struct compiler_t *comp, const char *in_file);
void comp_from_string(struct compiler_t *comp, const char *program, size_t program_size);

#endif
