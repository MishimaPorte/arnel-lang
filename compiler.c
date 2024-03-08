#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "hashmap.h"
#include "parser.h"
#include "sv.h"
#include "types.h"
struct compiler_t *init_compiler(struct compiler_t * comp)
{
    init_typesystem();
    comp->ids = 0;
    return comp;
};


void comp_set_output(struct compiler_t * comp, FILE *f)
{
    comp->output_file = f;
};

void output_val(struct compiler_t * comp, val_t v)
{
    if (v.imm) {
        if (v.t == T_DATA) {
            foutputf(comp, "$data_%lu", v.val);
        } else {
            foutputf(comp, "%zu", v.val);
        }
    } else {
        foutputf(comp, "%%x%zu", v.val);
    }
};

void output_val_t(struct compiler_t * comp, val_t v)
{
    foutputf(comp, "%c ", type_strings[v.t]);
    if (v.imm) {
        if (v.t == T_DATA) {
            foutputf(comp, "$data_%lu", v.val);
        } else {
            foutputf(comp, "%zu", v.val);
        }
    } else {
        foutputf(comp, "%%x%zu", v.val);
    }
};

static val_t __comp_expr(struct compiler_t * comp, ast_t *node)
{
    switch (node->t) {
    case __EXPR_TO_VOID: {
        return as_l(comp->temps_stack--);
    }; break;
    case A_CALL_FINISHED: {
        //TODO: implement function calls
        val_t *args = malloc(sizeof(val_t)*node->num_elems-2);
        for (size_t i = node->num_elems - 1; i >= 2; i--) {
            args[node->num_elems-1-i] = __comp_expr(comp, node->children[i].node);
            if (!args[node->num_elems-1-i].imm) comp->temps_stack--;
        }
        foutputf(comp, "  %%x%zu =l call $%.*s(", comp->temps_stack, sv_ptr_print(ast_as_sv(node)));
        for (size_t i = 2; i < node->num_elems; i++) {
            output_val_t(comp, args[i-2]);
            if (i != node->num_elems-1) foutputf(comp, ", ");
        }
        foutputf(comp, ")\n");
        return as_l(comp->temps_stack++);
    }; break;
    case A_INT_LITERAL:
        return ((val_t){T_L, 1, node->children[0].opaque});
    case A_STR_LITERAL:
        return as_data(node->children[1].opaque);
#define BINARY(t, op) \
    case t: {\
        val_t lhs = __comp_expr(comp, node->children[0].node);\
        val_t rhs = __comp_expr(comp, node->children[1].node);\
        if (!lhs.imm) comp->temps_stack--;\
        if (!rhs.imm) comp->temps_stack--;\
        foutputf(comp, "  %%x%zu =l " #op " ", comp->temps_stack);\
        output_val(comp, rhs);\
        foutputf(comp, ", ");\
        output_val(comp, lhs);\
        foutputf(comp, "\n");\
        return as_l(comp->temps_stack++);\
    }
    BINARY(A_INT_PLUS, add)
    BINARY(A_INT_MUL, mul)
    BINARY(A_INT_DIV, div)
    BINARY(A_INT_MINUS, sub)
    BINARY(A_INT_EQ, ceqw)
    BINARY(A_INT_NEQ, cnew)
#undef BINARY
    case A_IDENT: {
        comp->temps_stack++;
        local_t *local = um_get(comp->locals, ast_as_sv(node)->chars, ast_as_sv(node)->len);
        return as_l(local->arg);
    };
    case A_DECL_LOCAL: {
        local_t *local = malloc(sizeof(local_t));
        local->arg = comp->temps_stack++;
        um_set(comp->locals, ast_as_sv(node)->chars, ast_as_sv(node)->len, local);
        return as_l(comp->temps_stack);
    };
    case A_DECL_ASSIGNMENT: {
        val_t val = __comp_expr(comp, node->children[2].node);
        if (!val.imm) comp->temps_stack-=1;

        local_t *local = malloc(sizeof(local_t));
        local->arg = comp->temps_stack++;
        um_set(comp->locals, ast_as_sv(node)->chars, ast_as_sv(node)->len, local);
        foutputf(comp, "  %%x%zu =l add 0, ", local->arg);
        output_val(comp, val);
        foutputf(comp, "\n");
        return as_l(local->arg);
    };
    case A_COND: {
        val_t cond_value = __comp_expr(comp, node->children[0].node);
        comp->temps_stack-=1;
        uint32_t iftr = comp->ids++;
        uint32_t iffl = comp->ids++;
        uint32_t out = comp->ids++;
        foutputf(comp, "  jnz %%x%zu, @%.*s%d, ", cond_value, 4, "iftr", iftr);
        if (node->children[2].node != NULL) {
            foutputf(comp, "@%.*s%u\n", 4, "iffl", iffl);
        } else {
            foutputf(comp, "@%.*s%u\n", 3, "out", out);
        }
                
        foutputf(comp, "@%.*s%d\n", 4, "iftr", iftr);
        __comp_expr(comp, node->children[1].node);
        if (node->children[2].node != NULL) {
            foutputf(comp, "  jmp @%.*s%d\n", 3, "out", out);
            foutputf(comp, "@%.*s%d\n", 4, "iffl", iffl);
            __comp_expr(comp, node->children[2].node);
        };
        foutputf(comp, "@%.*s%d\n", 3, "out", out);
        return as_l(comp->temps_stack);
    }
    case A_ASSIGNMENT: {
        comp->temps_stack++;
        local_t *local = um_get(
            comp->locals, ((strview_t *)&(node->children[0].node->children[0]))->chars, ((strview_t *)&(node->children[0].node->children[0])) ->len);
        val_t val = __comp_expr(comp, node->children[1].node);
        foutputf(comp, "  %%x%zu =l add 0, ", local->arg);
        output_val(comp, val);
        foutputf(comp, "\n");
        if (!val.imm) comp->temps_stack-=1;
        return as_l(local->arg);
    }
    case A_RET: {
        val_t retted = __comp_expr(comp, node->children[0].node);
        comp->temps_stack-=1;
        foutputf(comp, "  ret %%x%zu\n", retted.val);
        return as_l(comp->temps_stack = 0);
    }
    case A_BLOCK: {
        for (size_t i = 0; i < node->children[0].opaque; i++)
            __comp_expr(comp, ((ast_t**)node->children[1].opaque)[i]);
    }; break;
    case A_WHILE: {
        uint32_t start = comp->ids++;
        uint32_t loop = comp->ids++;
        uint32_t breac = comp->ids++;
        foutputf(comp, "@%.*s%d\n", 5, "start", start);
        val_t cond = __comp_expr(comp, node->children[0].node);
        foutputf(comp, "  jnz ");
        output_val(comp, cond);
        foutputf(comp, ", @%s%d, @%s%d\n", "loop", loop, "breac", breac);
        foutputf(comp, "@%.*s%d\n", 4, "loop", loop);
        __comp_expr(comp, node->children[1].node);
        foutputf(comp, "  jmp @%s%d\n", "start", start);
        foutputf(comp, "@%.*s%d\n", 5, "breac", breac);
        comp->temps_stack-=1;
        return as_l(0);
    }
    case A_ROOT:
      break;
    default:
            printf("panika aa: %d\n", node->t);
            exit(69);
    }
};

static void comp_sym(struct compiler_t * comp, symbol_t *sym)
{
    comp->locals = malloc(sizeof(unordered_map_t));
    um_init(comp->locals);
    foutputf(comp, "export function w $%.*s(", sv_print(sym->symbol_name));
    type_t *func_type = get_type(sym->type);
    sig_t *sig = func_type->type_info;
    for (size_t i = 0; i < sig->arity; i++) {
        foutputf(comp, "l %%x%zu", i);
        if (i != sig->arity - 1) foutputf(comp, ", ");
    }
    foutputf(comp, ") {\n");
    foutputf(comp, "  @start\n");
    for (size_t i = 0; i < sym->data.body.num_elems; i++) __comp_expr(comp, sym->data.body.nodes[i]);
    foutputf(comp, "}\n\n");
}

void comp_sv(struct compiler_t * comp, strview_t sv)
{
    struct lexer_t l = {0};
    lexer_init(&l, sv);

    struct parser_t p = {0};
    init_parser(&p, &l);
    

    struct ast_t *root = parse(&p);
    for (size_t i = 0; i < root->num_elems; i++) {
        switch (root->children[i].node->t) {
        case A_DECL: {
            comp_sym(comp, (symbol_t*)root->children[i].node->children[0].opaque);
        }; break;
        case A_STR_LITERAL: { //INFO: strings are null terminated for now
            foutputf(
                comp,
                "data $data_%lu = { b \"%.*s\", b 0 }\n",
                root->children[i].node->children[1].opaque,
                root->children[i].node->children[0].opaque,
                (char*)root->children[i].node->children[1].opaque
            );
        }; break;
        default:
            fprintf(stderr, "unreachable\n");
            exit(69);
        }
    }
};

void comp_from_string(struct compiler_t *comp, const char *program, size_t program_size)
{
    strview_t sv = sv_make(program, program_size);
    comp_sv(comp, sv);
};

void comp_file(struct compiler_t *comp, const char *in_file)
{
    strbuf_t buf = {0};
    strbuf_append_file(&buf, in_file);
    comp_sv(comp, strbuf_get_sv(&buf, 0, buf.len));
    free(buf.buf);
};
