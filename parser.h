#ifndef ARNELPARSER
#define ARNELPARSER
#include "lexer.h"
#include "bytes.h"
#include "types.h"
#include <stddef.h>
#include <stdint.h>

typedef enum ast_type_t {
    A_INT_LITERAL,
    A_STR_LITERAL,
    A_INT_PLUS, A_INT_MUL, A_INT_MINUS, A_INT_DIV,

    A_INT_EQ, A_INT_NEQ,

    A_DECL, A_RET, A_CALL, A_CALL_FINISHED, A_IDENT,
    A_STMT, A_ROOT, A_DECL_LOCAL, A_DECL_ASSIGNMENT,

    A_COND, A_ELSE_CLAUSE, A_ASSIGNMENT,

    A_WHILE, A_BLOCK,

    __A_OPEN_PAREN, __A_CLOSE_PAREN, __EXPR_TO_VOID,

} ast_type_t;

typedef enum prec_t {
    PREC_STMT,
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR, // or
    PREC_AND, // and
    PREC_EQUALITY, // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // ! -
    PREC_CALL, // . () []
    PREC_PRIMARY
} prec_t;


typedef struct ast_t {
    ast_type_t t;
    prec_t prec;
    uint8_t ready;
    size_t num_elems;
    union {
        uintptr_t opaque;
        struct ast_t *node;
    } children[];
} ast_t;

#define ast_as_view(node) ((view_t*)&((node)->children[0].node))
#define ast_as_uint(node) ((uintmax_t*)((node)->children[0].node))

struct func_body {
    size_t num_elems;
    ast_t **nodes;
};

#define   __flag(n)         (0x01<<n)
#define   f_export(s)         (((s)->flags)&__flag(0))
#define   f_export_set(s)     (((s)->flags)|=__flag(0))
#define   f_export_unset(s)   (((s)->flags)&=__flag(0))
#define   f_const(s)          (((s)->flags)&__flag(1))
#define   f_const_set(s)      (((s)->flags)|=__flag(1))
#define   f_const_unset(s)    (((s)->flags)&=__flag(1))

typedef struct symbol_t {
    uint64_t flags;
    view_t symbol_name; //INFO: symbol name is the name by which an actual call is performed inside the executable.
                        // a link-time name. Aliases are made with scope thing.
    typeid_t type;
    size_t num_elems;
    union {
        struct func_body body;
    } as;
} symbol_t;


#define alloc_node(n) malloc(sizeof(ast_t)+sizeof(uintptr_t)*(n))

typedef struct ast_stack {
    size_t cap, len;
    struct ast_t **stack;
} asts_t;

void put_ast(asts_t *st, ast_t *n);
ast_t *pop_ast(asts_t *st);
ast_t *peek_ast(asts_t *s);
#define popval(p) (pop_ast(&(p)->values))
#define peekval(p) (peek_ast(&(p)->values))
#define putval(p, v) (put_ast(&(p)->values, v))

#define popop(p) (pop_ast(&(p)->operators))
#define peekop(p) (peek_ast(&(p)->operators))
#define putop(p, v) (put_ast(&(p)->operators, v))

#define popstmt(p) (pop_ast(&(p)->block))
#define peekstmt(p) (peek_ast(&(p)->block))
#define putstmt(p, v) (put_ast(&(p)->block, v))


struct parser_t {
    /* INFO: some flags
     * 0x01 is set if the statement now compiled is prepended with the export keyword
     * 0x02 if now is a struct compilation
     * */
    uint16_t flags;

    asts_t block, values, operators;

    struct lexer_t *l;

    symbol_t *now_symbol;
};

struct parser_t *init_parser(struct parser_t *p, struct lexer_t *l);

ast_t *parse(struct parser_t *p);

#endif
