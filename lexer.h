#ifndef ARNELLEXER
#define ARNELLEXER

#include "da.h"
#include "bytes.h"
#include <stddef.h>
#include <stdint.h>


enum token_type_t {
    L_INTEGER, L_FLOAT,
    L_STRING,
    L_IDENT,
    L_MULT_EQ,
    L_PLUS, L_MINUS, L_PLUS_EQ, L_MINUS_EQ, L_EQ, L_BANG, L_DIV, L_DIV_EQ, L_BACKSLASH,
    L_PLUS2, L_MINUS2, L_EQ2, L_BANG_EQ, L_COMMA,
    L_LESS, L_GREATER, L_EQ_GREATER, L_EQ_LESS,
    L_ARROW_L, L_ARROW_R, L_COLON, L_SEMICOLON,
    L_RPAREN, L_LPAREN, L_RCURLY, L_LCURLY, L_RBRACK, L_LBRACK,

    L_PIPE,

    L_BOR, L_BAND, L_BOR_EQ, L_BAND_EQ, L_HAT, L_HAT_EQ, L_TILDA,
    L_QUESTION, L_QUOT, L_QUOT2,

    L_TRUE, L_FALSE,

    L_DOT, L_QDOT, L_STAR, L_REF,

    L_BACKTICK,

    //INFO: keywords
    L_SYMBOL, L_STRUCT, L_CONST, L_VAR,
    L_DOG, L_PERCENT, L_EXPORT, L_LOCAL, L_RETURN,
    L_VOID, L_NOTHING, L_UNDEFINED, L_NAN,
    L_EXTERN, L_STATIC, L_NAME, L_NAMES, L_AS,

    L_IMPORT,

    L_IF, L_ELSE, L_ELIF, L_WHILE, L_FOR, L_IN,

    L_EOF
};

typedef struct token_t {
    view_t lexeme;
    uint64_t type;
#ifdef ARNEL_DEBUG_BUILD
    size_t line, col;
#endif
} token_t;

typedef struct tokens {
    token_t **items;
    size_t cap, len;
} tokens;

tokens *tokens_append(tokens *da, token_t *item);
token_t *tokens_get(tokens *da, size_t i);
tokens *tokens_init(tokens *da, size_t i);
tokens *tokens_remove(tokens *da, size_t i, token_t **out);

struct lexer_t {
    view_t text;
    tokens toks;
#ifdef ARNEL_DEBUG_BUILD
    size_t line, col;
#endif
};

#define cur(l) ((l)->text.chars[0])

struct lexer_t *lexer_init(struct lexer_t *l, view_t v);
token_t *next_token(struct lexer_t *l);
// void push_token(struct lexer_t *l, token_t *tok);
token_t *peek_token(struct lexer_t *l, size_t lookahead);

#endif
