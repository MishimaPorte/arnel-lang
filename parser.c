#include "parser.h"
#include "lexer.h"
#include "bytes.h"
#include "types.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#define NOT_IMPLEMENTED do { printf("NOT IMPLEMENTED: %s:%d\n", __FILE__, __LINE__); exit(69); } while(0)
#define PANIKA(msg) do { printf("PANIKA: %s: %s:%d\n", msg, __FILE__, __LINE__); exit(69); } while(0)

#define must_match(p, t, msg) if (next_token((p)->l)->type != t) PANIKA(msg)
#define check_next(p, t, msg) if (peek_token((p)->l, 1)->type != t) PANIKA(msg)

struct parser_t *init_parser(struct parser_t *p, struct lexer_t *l)
{
    p->l = l;
    p->values.cap = 12;
    if ((p->values.stack = malloc(sizeof(ast_t*)*12)) == NULL) return NULL;

    p->operators.cap = 12;
    if ((p->operators.stack = malloc(sizeof(ast_t*)*12)) == NULL) return NULL;

    p->block.cap = 12;
    if ((p->block.stack = malloc(sizeof(ast_t*)*12)) == NULL) return NULL;

    return p;
};

static typeid_t parse_type(struct parser_t *p)
{
    //TODO: not single-token types: like arrays or pointer types
    token_t *tok = next_token(p->l);
    switch (tok->type) {
        case L_IDENT:
            return get_type_for_name(tok->lexeme)->__type_id;
        case L_LPAREN: { //INFO: parsing function signature (it is a type, a function type)
            // TODO: create a string representation of function signatures
            // strview_t func_sig = tok->lexeme;
            type_t *sig_type = alloc_type(p->now_symbol->symbol_name);
            types_t temp = {0};
            while ((tok = next_token(p->l))->type != L_RPAREN) {
                if (tok->type == L_COMMA) continue;
                ast_t *node = alloc_node(2);
                node->t = A_DECL_LOCAL;
                node->num_elems = 2;
                node->prec = PREC_STMT;
                *(view_t*)&node->children[0].opaque = tok->lexeme;
                free(tok);
                tok = next_token(p->l);
                putval(p, node);
                if (tok->type != L_COLON) PANIKA("need type declaration after func parameter");
                put_type(&temp, parse_type(p));
            }
            sig_t *sig = malloc(sizeof(sig_t) + sizeof(typeid_t)*temp.len);
            for (size_t i = 0; i < temp.len; i++) sig->arg_types[i] = temp.items[i];
            sig->arity = temp.len;
            free(temp.items);
            if (next_token(p->l)->type != L_COLON) PANIKA("need colon before the return type");
            sig->return_type_id  = parse_type(p);
            sig_type->type_info = sig;
            return sig_type->__type_id;
        }; break;
    }
    return __BAD_TYPE;
};

static void sub_expr(struct parser_t *p)
{
    ast_t *prev = popop(p);
    switch (prev->t) {
    case A_INT_PLUS: case A_INT_MUL: case A_INT_DIV: case A_INT_MINUS:
    case A_INT_EQ: case A_INT_NEQ: {
        prev->children[0].node = popval(p);
        prev->children[1].node = popval(p);
    }; break;
    case A_ASSIGNMENT: { //TODO: implement assignment
        prev->children[1].node = popval(p);
    }; break;
    case A_DECL_ASSIGNMENT: { //TODO: implement assignment
        prev->children[2].node = popval(p);
    }; break;
    case A_RET: {
        prev->children[0].node = popval(p);
    }; break;
    default: {
        printf("not implemented what: %d\n", prev->t);
        NOT_IMPLEMENTED;
    }; break;
    }
    putval(p, prev);
}

static void __parse(struct parser_t *p)
{
    ast_t *node;
    token_t *tok = peek_token(p->l, 1);
    switch (tok->type) {
        case L_ELSE: { //not needed here too???
            ast_t *node = alloc_node(0);
            node->t = A_ELSE_CLAUSE;
            node->num_elems = 0;
            node->ready = 1;
            node->prec = PREC_STMT;
            // memset(node->children, 0, sizeof(uintptr_t)*1);
            putop(p, node);
        }; break;
        case L_SEMICOLON: { 
            while (p->operators.len != 0 && peekop(p)->prec != PREC_STMT)  // INFO: semicolons terminate statements;
                sub_expr(p);                                               // statement borders are delimeted by nodes

            if (p->operators.len != 0 && peekop(p)->prec == PREC_STMT && peekop(p)->ready == 1)
                do switch (peekop(p)->t) {
                    case A_COND: {
                        peekop(p)->children[1].node = popval(p);
                        putval(p, popop(p));
                    }; break;
                    case A_ELSE_CLAUSE: {
                        popop(p);
                        ast_t *conditional = popop(p);
                        conditional->children[2].node = popval(p);
                        conditional->children[1].node = popval(p);
                        putval(p, conditional);
                    }; break;
                    case A_RET: {
                        peekop(p)->children[0].node = popval(p);
                        putval(p, popop(p));
                    }; break;
                    case A_WHILE: {
                        peekop(p)->children[1].node = popval(p);
                        putval(p, popop(p));
                    }; break;
                    default: {
                        goto out;
                    }; break;
                } while (p->operators.len != 0 && peekop(p)->prec == PREC_STMT && peekop(p)->ready == 1); 
            else if (peekval(p)->t != A_DECL_LOCAL && peekval(p)->t != A_DECL_ASSIGNMENT){
                fprintf(stderr, "expr voidance, tok: %zu/%zu\n", tok->line, tok->col);
                ast_t *node = alloc_node(0);
                node->t = __EXPR_TO_VOID;
                node->num_elems = 0;
                node->ready = 1;
                node->prec = PREC_STMT;
                putval(p, node);
            };
        }; break;  // with statement precedence value
        case L_LCURLY: { // blocks are things
            asts_t values = p->values, operators = p->operators;
            p->values.cap = 12;
            p->values.len = 0;
            p->values.stack = malloc(sizeof(ast_t*)*12);
            p->operators.cap = 12;
            p->operators.len = 0;
            p->operators.stack = malloc(sizeof(ast_t*)*12);
            next_token(p->l);

            while (peek_token(p->l, 1)->type != L_RCURLY) __parse(p);

            ast_t *node = alloc_node(2);
            node->t = A_BLOCK;
            node->num_elems = 2;
            node->ready = 1;
            node->prec = PREC_STMT;
            if (peekval(p)->t == __EXPR_TO_VOID) {
                popval(p);
                fprintf(stderr, "doing pop\n");
            };
            node->children[0].opaque = p->values.len;
            node->children[1].opaque = (uintptr_t)p->values.stack;
            free(p->operators.stack);
            p->values = values;
            p->operators = operators;
            putval(p, node);
        }; break;
        case L_WHILE: {
            ast_t *node = alloc_node(2);
            node->t = A_WHILE;
            node->num_elems = 2;
            node->ready = 1;
            node->prec = PREC_STMT;
            putop(p, node);
        }; break;
        case L_VAR: { 
            token_t *id = peek_token(p->l, 2);
            if (id->type != L_IDENT) {
                PANIKA("need identifier after var declaration");
            } else next_token(p->l);
            ast_t *node;
            if (peek_token(p->l, 2)->type == L_EQ) {
                next_token(p->l);
                node = alloc_node(3);
                node->t = A_DECL_ASSIGNMENT;
                node->num_elems = 3;
                node->prec = PREC_PRIMARY;
                putop(p, node);
            } else {
                node = alloc_node(2);
                node->t = A_DECL_LOCAL;
                node->num_elems = 2;
                node->prec = PREC_STMT;
                putval(p, node);
            }
            *(view_t*)&node->children[0] = id->lexeme;
        }; break;
        case L_EQ: { // TODO: implement assignment
            ast_t *node = alloc_node(2); // lvalue and rvalue
            node->t = A_ASSIGNMENT;
            node->num_elems = 2;
            node->prec = PREC_ASSIGNMENT;
            node->children[0].node = popval(p);
            putop(p, node);
        }; break;
        case L_IF: {
           fprintf(stderr, "here!\n");
            ast_t *node = alloc_node(3);
            node->t = A_COND;
            node->num_elems = 3;
            node->ready = 1;
            node->prec = PREC_STMT;
            memset(node->children, 0, sizeof(uintptr_t)*3);
            putop(p, node);
        }; break;
        case L_RETURN:
            node = alloc_node(1);
            node->t = A_RET;
            node->ready = 1;
            node->num_elems = 1;
            node->prec = PREC_STMT;
            putop(p, node);
            break;
        case L_IDENT: {
            node = alloc_node(2);
            node->t = A_IDENT;
            node->num_elems = 2;
            node->prec = PREC_PRIMARY;
            *(view_t*)&node->children[0].opaque = tok->lexeme; 
            putval(p, node);
        }; break;
        case L_RPAREN: { //INFO: parsing parentheses for dijkstras thing
            while (p->operators.len != 0 && peekop(p)->t != __A_OPEN_PAREN)
                sub_expr(p);
            popop(p);
            if (p->operators.len != 0) switch (peekop(p)->t) {
                case A_COND: case A_WHILE: {
                    peekop(p)->children[0].node = popval(p); // assigning the condition thing
                }; break;
                case A_CALL: {
                    popop(p);
                    asts_t *args = calloc(1, sizeof(asts_t));
                    while (p->values.len != 0 && peekval(p)->t != A_CALL)
                        put_ast(args, popval(p));
                    ast_t *old_node = popval(p);
                    ast_t *func = alloc_node(args->len + 2); //INFO: allocating a new function call node
                                                         // to create space for children
                    *(view_t*)&func->children[0].opaque = *(view_t*)&old_node->children[0].opaque; 
                    memcpy(&func->children[2], args->stack, sizeof(void*)*args->len);
                    func->t = A_CALL_FINISHED;
                    func->num_elems = args->len + 2;
                    func->prec = PREC_NONE;
                    free(args->stack);
                    putval(p, func);
                }; break;
            };
        }; break;
        case L_LPAREN: {
            //TODO: parse function call
            if (p->values.len != 0) switch (peekval(p)->t) {
                case A_IDENT: { //INFO: changing the type of identifer node to a call node
                    peekval(p)->t = A_CALL;
                    peekval(p)->prec = PREC_CALL;
                    putop(p, peekval(p));
                }; break;
                case A_COND: { //INFO: condition handling (nothing to do here since
                               // the paren after the if has no meaning)
                               // peekval(p)->t = A_CALL;
                               // peekval(p)->prec = PREC_CALL;
                               // putop(p, peekval(p));
                }; break;
                default: break;
            };
            node = alloc_node(0);
            node->t = __A_OPEN_PAREN;
            node->num_elems = 0;
            node->prec = PREC_NONE;
            putop(p, node);
        }; break;
        case L_STRING: {
            node = alloc_node(2);
            node->t = A_STR_LITERAL;
            node->num_elems = 2;
            node->prec = PREC_NONE;
            node->children[0].opaque = tok->lexeme.len;
            node->children[1].opaque = (uintptr_t)tok->lexeme.chars;
            putval(p, node);
            putstmt(p, node);
        }; break;
        case L_INTEGER:
            node = alloc_node(1);
            node->t = A_INT_LITERAL;
            node->num_elems = 1;
            node->prec = PREC_NONE;
            node->children[0].opaque = strtol(tok->lexeme.chars, NULL, 10);
            putval(p, node);
            break;
#define BINARY(type, _prec, num) \
            node = alloc_node(num);                                  \
            node->t = type;                                          \
            node->num_elems = num;                                   \
            node->prec = _prec;                                      \
            if (p->operators.len != 0 && (peekop(p)->prec >= _prec)) \
                sub_expr(p);                                         \
            putop(p, node);                                          \
            break;
        case L_PLUS: BINARY(A_INT_PLUS, PREC_TERM, 2)
        case L_STAR: BINARY(A_INT_MUL, PREC_TERM, 2)
        case L_EQ2: BINARY(A_INT_EQ, PREC_COMPARISON, 2)
        case L_BANG_EQ: BINARY(A_INT_NEQ, PREC_COMPARISON, 2)
        case L_MINUS: BINARY(A_INT_MINUS, PREC_TERM, 2)
        case L_DIV: BINARY(A_INT_DIV, PREC_TERM, 2)
#undef BINARY
        default: break;
    }
out:
    next_token(p->l);
};

static ast_t *parse_top_level(struct parser_t *p)
{
    if (p->now_symbol != NULL) PANIKA("two declrs");
    p->now_symbol = calloc(sizeof(symbol_t), 1);
    token_t *tok = next_token(p->l); //TODO: a memory leak!
    if (tok->type == L_EOF) return NULL;
    while (tok->type != L_IDENT) {
        switch (tok->type) {
        case L_EXPORT:
            if (f_export(p->now_symbol)) PANIKA("two exports in a row");
            f_export_set(p->now_symbol);
            break;
         case L_LOCAL:
            break;
         default:
            printf("token: %d\n", tok->type);
            PANIKA("not allowed token here");
        }
        tok = next_token(p->l);
    };
    p->now_symbol->symbol_name = tok->lexeme;
    if (next_token(p->l)->type != L_EQ) PANIKA("need equal after identifier in symbol declaration");
    p->now_symbol->type = parse_type(p);
    if (next_token(p->l)->type != L_ARROW_R) PANIKA("need arrow after return type and before the body");
    if (next_token(p->l)->type != L_LCURLY) PANIKA("need arrow after return type and before the body");

    while (peek_token(p->l, 1)->type != L_RCURLY) {
        __parse(p);
    };
    next_token(p->l); //consuming a curly

    if (next_token(p->l)->type != L_SEMICOLON) PANIKA("semicolon after declaration needed");
    p->now_symbol->as.body.num_elems = p->values.len;
    p->now_symbol->as.body.nodes = malloc(sizeof(void*)*p->values.len);
    memcpy(p->now_symbol->as.body.nodes, p->values.stack, p->values.len * sizeof(void*));
    ast_t *root = alloc_node(1);
    root->t = A_DECL;
    root->num_elems = 1;
    root->children[0].opaque = (uintptr_t)p->now_symbol;
    p->operators.len = 0;
    p->values.len = 0;
    return root;
}

ast_t *parse(struct parser_t *p)
{
    ast_t *sym = NULL;
    do {
        sym = parse_top_level(p);
        p->now_symbol = NULL;
        if (sym != NULL) putstmt(p, sym);
        else break;
    } while (1);
    ast_t *root = alloc_node(p->block.len);
    root->t = A_ROOT;
    root->num_elems = p->block.len;
    memcpy(root->children, p->block.stack, p->block.len * sizeof(void*));
    return root;
};


ast_t *pop_ast(asts_t *s)
{
    if (s->len == 0)
        return NULL;
    return s->stack[--s->len];
};

ast_t *peek_ast(asts_t *s)
{
    if (s->len == 0)
        return NULL;
    return s->stack[s->len - 1];
};

void put_ast(asts_t *s, ast_t *n)
{
    if (s->len == s->cap) {
        s->stack = realloc(s->stack, (s->cap*=2)*sizeof(void*));
    }
    s->stack[s->len++] = n;
    return;
};

