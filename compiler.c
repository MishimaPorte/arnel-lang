#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "hashmap.h"
#include "parser.h"
#include "bytes.h"
#include "types.h"

uintptr_t scope_get_scoped(struct scope *scope, view_t name, uint8_t type)
{
    switch (type) {
        case SCOPED_TYPE:
            return (uintptr_t)um_get(&scope->types, name.chars, name.len); break;
        case SCOPED_MODULE:
            return (uintptr_t)um_get(&scope->modules, name.chars, name.len); break;
        case SCOPED_SYMBOL:
            return (uintptr_t)um_get(&scope->symbols, name.chars, name.len); break;
        default: return 0;
    }
};

struct scope *scope_put_scoped(struct scope *scope, view_t name, uintptr_t item, uint8_t type)
{
    switch (type) {
        case SCOPED_TYPE:
            um_set(&scope->types, name.chars, name.len, (void*)item); break;
        case SCOPED_MODULE:
            um_set(&scope->modules, name.chars, name.len, (void*)item); break;
        case SCOPED_SYMBOL:
            um_set(&scope->symbols, name.chars, name.len, (void*)item); break;
    }

    return scope;
};

static void init_scope(struct compiler_t *comp)
{
    comp->scope.types = *um_init(&comp->scope.types);
    comp->scope.modules = *um_init(&comp->scope.modules);
    comp->scope.symbols = *um_init(&comp->scope.symbols);

    //INFO: this is not the best solution
    set_scope_type(comp, view_from_literal("float"), TYPE_FLOAT);
    set_scope_type(comp, view_from_literal("str"), TYPE_STRING);
    set_scope_type(comp, view_from_literal("cstr"), TYPE_CSTRING);
    set_scope_type(comp, view_from_literal("bool"), TYPE_BOOL);
    set_scope_type(comp, view_from_literal("nothing"), TYPE_NOTHING);
    set_scope_type(comp, view_from_literal("void"), TYPE_VOID);
    set_scope_type(comp, view_from_literal("module"), TYPE_MODULE);
    set_scope_type(comp, view_from_literal("int"), TYPE_INTEGER);
    set_scope_type(comp, view_from_literal("double"), TYPE_DOUBLE);
    set_scope_type(comp, view_from_literal("type"), TYPE_TYPE);

    type_t *sig_type = alloc_type(view_from_literal("print_format"));
    sig_t *sig = malloc(sizeof(sig_t) + sizeof(typeid_t)*2);
    sig->arity = 2;
    sig->arg_types[0] = TYPE_STRING;
    sig->arg_types[1] = TYPE_VARIADIC;
    sig->return_type_id = TYPE_INTEGER;

    symbol_t *sym = calloc(sizeof(symbol_t), 1);
    sym->type = sig_type->__type_id;
    sym->symbol_name = view_from_literal("printf");
    //TODO: do proper extern declarations for this kind of things
    alias_symbol(comp, sym, "print_format");
    alias_symbol(comp, sym, "print");
};

struct compiler_t *init_compiler(struct compiler_t * comp)
{
    init_typesystem();
    init_scope(comp);
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

static val_t __comp_expr(struct compiler_t * comp, ast_t *node);
static val_t __comp_func_call(struct compiler_t *comp, ast_t *node);
static val_t __comp_cond(struct compiler_t *comp, ast_t *node);
static val_t __comp_while(struct compiler_t *comp, ast_t *node);
static val_t __comp_assignment(struct compiler_t *comp, ast_t *node);
static val_t __comp_decl_assignment(struct compiler_t *comp, ast_t *node);

static val_t __comp_assignment(struct compiler_t *comp, ast_t *node)
{
    comp->temps_stack++;
    local_t *local = um_get(comp->locals, ((view_t *)&(node->children[0].node->children[0]))->chars, ((view_t *)&(node->children[0].node->children[0])) ->len);
    val_t val = __comp_expr(comp, node->children[1].node);
    foutputf(comp, "  %%x%zu =l add 0, ", local->arg);
    output_val(comp, val);
    foutputf(comp, "\n");
    if (!val.imm) comp->temps_stack-=1;
    return as_l(local->arg);
}
static val_t __comp_decl_assignment(struct compiler_t *comp, ast_t *node)
{
    val_t val = __comp_expr(comp, node->children[2].node);
    if (!val.imm) comp->temps_stack-=1;
    local_t *local = malloc(sizeof(local_t));
    local->arg = comp->temps_stack++;
    um_set(comp->locals, ast_as_view(node)->chars, ast_as_view(node)->len, local);
    foutputf(comp, "  %%x%zu =l add 0, ", local->arg);
    output_val(comp, val);
    foutputf(comp, "\n");
    return as_l(local->arg);
}

static val_t __comp_while(struct compiler_t *comp, ast_t *node)
{
    uint32_t start = comp->ids++;
    uint32_t loop = comp->ids++;
    uint32_t breac = comp->ids++;
    foutputf(comp, "@%.*s%d\n", 5, "start", start);
    val_t cond = __comp_expr(comp, node->children[0].node);
    foutputf(comp, "  jnz ");
    output_val(comp, cond);
    foutputf(comp, ", @%s%d, @%s%d\n", "loop", loop, "breac", breac);
    foutputf(comp, "@%.*s%d\n", 4, "loop", loop);
    val_t r = __comp_expr(comp, node->children[1].node);
    foutputf(comp, "  jmp @%s%d\n", "start", start);
    foutputf(comp, "@%.*s%d\n", 5, "breac", breac);
    return r;
}

static val_t __comp_cond(struct compiler_t *comp, ast_t *node)
{

    val_t cond_value = __comp_expr(comp, node->children[0].node);
    if (!cond_value.imm) comp->temps_stack-=1;
    uint32_t iftr = comp->ids++;
    uint32_t iffl = comp->ids++;
    uint32_t out = comp->ids++;
    foutputf(comp, "  jnz ");
    output_val(comp, cond_value);
    foutputf(comp, ", @%.*s%d, ", 4, "iftr", iftr);
    int is_one = 0;
    if (node->children[2].node != NULL) {
        foutputf(comp, "@%.*s%u\n", 4, "iffl", iffl);
    } else {
        is_one = 1;
        foutputf(comp, "@%.*s%u\n", 3, "out", out);
    }
            
    foutputf(comp, "@%.*s%d\n", 4, "iftr", iftr);
    val_t i = __comp_expr(comp, node->children[1].node);
    if (!i.imm) comp->temps_stack-=1;
    fprintf(stderr, "doing shit, temps stack: %zu\n", comp->temps_stack);
    if (i.t != T_RET) {
        foutputf(comp, "  %%x%zu =l add 0, ", comp->temps_stack);
        output_val(comp, i);
        foutputf(comp, "\n");
    };
    if (node->children[2].node != NULL) {
        if (i.t != T_RET) {
            foutputf(comp, "  jmp @%.*s%d\n", 3, "out", out);
        };
        foutputf(comp, "@%.*s%d\n", 4, "iffl", iffl);
        val_t r = __comp_expr(comp, node->children[2].node);
        if (!r.imm) comp->temps_stack-=1;
        if (r.t != T_RET) {
            foutputf(comp, "  %%x%zu =l add 0, ", comp->temps_stack);
            output_val(comp, r);
            foutputf(comp, "\n");
            // if (!r.imm) comp->temps_stack-=1;
        };
    } else {
        if (!is_one) foutputf(comp, "  %%x%zu =l add 0, 0\n", comp->temps_stack);
    };
    // if (!i.imm) comp->temps_stack-=1;
    if (i.t != T_RET || is_one) foutputf(comp, "@%.*s%d\n", 3, "out", out);
    return as_l(comp->temps_stack++);
}

static val_t __comp_func_call(struct compiler_t *comp, ast_t *node)
{
    symbol_t *sym = get_scope_symbol(comp, *ast_as_view(node));
    if (sym == NULL) { //FIXME: fix this madness
        printf("symbol not found: %.*s\n", view_ptr_print(ast_as_view(node)));
        exit(69);
    } 

    val_t *args = malloc(sizeof(val_t)*node->num_elems-2);
    for (size_t i = node->num_elems - 1; i >= 2; i--) {
        args[node->num_elems-1-i] = __comp_expr(comp, node->children[i].node);
        if (!args[node->num_elems-1-i].imm) comp->temps_stack--;
    }
    foutputf(comp, "  %%x%zu =l call $%.*s(", comp->temps_stack, view_print(sym->symbol_name));
    for (size_t i = 2; i < node->num_elems; i++) {
        output_val_t(comp, args[i-2]);
        if (i != node->num_elems-1) foutputf(comp, ", ");
    }
    foutputf(comp, ")\n");
    return as_l(comp->temps_stack++);
};

static val_t __comp_expr(struct compiler_t * comp, ast_t *node)
{
    switch (node->t) {

    case __EXPR_TO_VOID: return as_l(comp->temps_stack--); // INFO: void the temp stack after an expression statement
    case A_CALL_FINISHED: return __comp_func_call(comp, node);
    case A_INT_LITERAL: return ((val_t){T_L, 1, node->children[0].opaque});
    case A_STR_LITERAL: return as_data(node->children[1].opaque);
    case A_IDENT: {
        comp->temps_stack++;
        local_t *local = um_get(comp->locals, ast_as_view(node)->chars, ast_as_view(node)->len);
        return as_l(local->arg);
    };
    case A_DECL_LOCAL: {
        local_t *local = malloc(sizeof(local_t));
        local->arg = comp->temps_stack++;
        um_set(comp->locals, ast_as_view(node)->chars, ast_as_view(node)->len, local);
        return as_l(comp->temps_stack);
    };
    case A_COND: return __comp_cond(comp, node);
    case A_DECL_ASSIGNMENT: return __comp_decl_assignment(comp, node);
    case A_ASSIGNMENT: return __comp_assignment(comp, node);
    case A_RET: {
        val_t retted = __comp_expr(comp, node->children[0].node);
        comp->temps_stack-=1;
        foutputf(comp, "  ret ");
        output_val(comp, retted);
        foutputf(comp, "\n");
        comp->temps_stack = 0;
        return ret;
    }
    case A_BLOCK: {
        val_t r;
        for (size_t i = 0; i < node->children[0].opaque; i++) {
            r = __comp_expr(comp, ((ast_t**)node->children[1].opaque)[i]);
            // if (!r.imm) comp->temps_stack--;
        }
        return r;
    }; break;
    case A_WHILE: {
        return __comp_while(comp, node);
    }
    case A_ROOT:
      break;
    default:
        printf("panika aa: %d\n", node->t);
        exit(69);
//INFO: binary operators are all the same so we declare
// them with a local macro for the sake of
// a) economy and b) readability;
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
    }
};

static void comp_sym(struct compiler_t * comp, symbol_t *sym)
{
    comp->locals = malloc(sizeof(unordered_map_t));
    um_init(comp->locals);
    foutputf(comp, "export function l $%.*s(", view_print(sym->symbol_name));
    type_t *func_type = get_type(sym->type);
    sig_t *sig = func_type->type_info;
    for (size_t i = 0; i < sig->arity; i++) {
        foutputf(comp, "l %%x%zu", i);
        if (i != sig->arity - 1) foutputf(comp, ", ");
    }
    foutputf(comp, ") {\n");
    foutputf(comp, "  @start\n");
    for (size_t i = 0; i < sym->as.body.num_elems; i++) __comp_expr(comp, sym->as.body.nodes[i]);
    foutputf(comp, "}\n\n");
}

void comp_view(struct compiler_t * comp, view_t view)
{
    struct lexer_t l = {0};
    lexer_init(&l, view);

    struct parser_t p = {0};
    init_parser(&p, &l);
    

    struct ast_t *root = parse(&p);
    for (size_t i = 0; i < root->num_elems; i++) {
        switch (root->children[i].node->t) {
        case A_DECL: {
            set_scope_symbol(comp, (symbol_t*)root->children[i].node->children[0].opaque);
        }; break;
        case A_STR_LITERAL: { //INFO: strings are null terminated for now
            // foutputf(
            //     comp,
            //     "data $data_%lu = { b \"%.*s\", b 0 }\n",
            //     root->children[i].node->children[1].opaque,
            //     root->children[i].node->children[0].opaque,
            //     (char*)root->children[i].node->children[1].opaque
            // );
        }; break;
        default:
            fprintf(stderr, "unreachable\n");
            exit(69);
        }
    }
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
    view_t view = view_make(program, program_size);
    comp_view(comp, view);
};

void comp_file(struct compiler_t *comp, const char *in_file)
{
    buf_t buf = {0};
    buf_write_file(&buf, in_file);
    comp_view(comp, buf_get_view(&buf, 0, buf.len));
    free(buf.buf);
};
