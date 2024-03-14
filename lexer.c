#include "bytes.h"
#include "lexer.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct lexer_t *lexer_init(struct lexer_t *l, view_t v)
{
    tokens_init(&l->toks, 1);
    l->text = v;
#ifdef ARNEL_DEBUG_BUILD
    l->line = 0;
    l->col = 0;
#endif
    return l;
};

static void lex_num(struct lexer_t *l, token_t *tok)
{
    int is_float = 0;
    size_t i = 0;
    while (1) {
        l->col++;
        if (isdigit(l->text.chars[i])) i++;
        else if (l->text.chars[i] == '.') {
            i++;
            if (!is_float) {is_float = 1; continue;}
            tok->lexeme.chars = NULL;
            return;
        } else break;
    }
    tok->type = is_float ? L_FLOAT : L_INTEGER;
    tok->lexeme = view_make(&cur(l), i);
    l->text.chars+=i;
    l->text.len-=i;
};

static int __chop_predicate_word(char);
#ifdef ARNEL_DEBUG_BUILD
static struct lexer_t *lexer;
static int __chop_predicate_word(char c) {
    if (c == '\n') {
        lexer->col = 0;
        lexer->line++;
    } else lexer->col++;
    return isalnum(c) || c == '_';
};
#else
static int __chop_predicate_word(char c) { return !isalnum(c) && !(c != '_'); };
#endif
static void ident_check_kw(token_t *t, const char *word, size_t len, enum token_type_t ifmatch)
{
    size_t wlen = strlen(word);
    if (t->lexeme.len == len + wlen && memcmp(t->lexeme.chars + len, word, t->lexeme.len - len) == 0) t->type = ifmatch;
    else t->type = L_IDENT;
}

static void check_kw(token_t *t)
{
    switch (*t->lexeme.chars) {
    case 'c':
        if (t->lexeme.len > 1) {
            switch (t->lexeme.chars[1]) {
            case 'o': ident_check_kw(t, "nst", 2, L_CONST); break;
            default: t->type = L_IDENT; break;
            }
        } else t->type = L_IDENT; 
        break;
    case 'e':
        if (t->lexeme.len > 1) {
            switch (t->lexeme.chars[1]) {
            case 'l': ident_check_kw(t, "se", 2, L_ELSE); break;
            case 'x': ident_check_kw(t, "port", 2, L_EXPORT); break;
            default: t->type = L_IDENT; break;
            }
        } else t->type = L_IDENT; 
        break;
    case 'i':
        if (t->lexeme.len == 2) {
            switch (t->lexeme.chars[1]) {
            case 'f': t->type = L_IF; break;
            default: t->type = L_IDENT; break;
            }
        } else t->type = L_IDENT;
        break;
    case 'n': {ident_check_kw(t, "othing", 1, L_NOTHING); break;};
    case 'r': {ident_check_kw(t, "eturn", 1, L_RETURN); break;};
    case 't': {ident_check_kw(t, "rue", 1, L_TRUE); break;};
    case 'v':
        if (t->lexeme.len > 1) {
            switch (t->lexeme.chars[1]) {
            case 'a': ident_check_kw(t, "r", 2, L_VAR); break;
            case 'o': ident_check_kw(t, "id", 2, L_VOID); break;
            default: t->type = L_IDENT; break;
            }
        } else t->type = L_IDENT; 
        break;
    case 'w': {ident_check_kw(t, "hile", 1, L_WHILE); break;};
    case 'f':
        if (t->lexeme.len > 1) {
            switch (t->lexeme.chars[1]) {
            case 'a': ident_check_kw(t, "lse", 2, L_FALSE); break;
            case 'o': ident_check_kw(t, "r", 2, L_FOR); break;
            default: t->type = L_IDENT; break;
            }
        } else t->type = L_IDENT; 
        break;
    default: t->type = L_IDENT;
    }
};
static void lex_word(struct lexer_t *l, token_t *tok) {
#ifdef ARNEL_DEBUG_BUILD
    lexer = l;
#endif
    tok->lexeme = (view_t){l->text.chars}; // INFO: we NOLINT here because the length is assigned later
    view_chop_predicate(&l->text, __chop_predicate_word);
    tok->lexeme.len = l->text.chars - tok->lexeme.chars;
    if (tok->lexeme.len == 0) tok->type = L_EOF;
    else check_kw(tok);
};

static inline void trim_left(struct lexer_t *l)
{
#ifdef ARNEL_DEBUG_BUILD
    while (isspace(cur(l))) {
        l->text.chars++;
        l->text.len--;
        if (cur(l) == '\n') {
            l->col = 0;
            l->line++;
        } else l->col++;
    }
#else
    l->text = view_trim_left(l->text);
#endif
}
static token_t *__next_token(struct lexer_t *l)
{
    trim_left(l);
    token_t *tok = malloc(sizeof(token_t));
#ifdef ARNEL_DEBUG_BUILD
    tok->line = l->line;
    tok->col = l->col;
#endif
    tok->lexeme.chars = NULL;
    if (l->text.len == 0) {
        tok->type = L_EOF;
        return tok;
    };
    switch (*l->text.chars) {
        case '"': {
            l->text.chars++;
            l->text.len--;
            size_t i = 0;
            tok->lexeme.chars = l->text.chars;
            while (*l->text.chars != '"') {
                i++;
                l->text.chars++;
                l->text.len--;
            };
            tok->lexeme.len = i;
            tok->type = L_STRING;
            l->text.chars++;
            l->text.len--;
        }; break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '0':
            lex_num(l, tok); break;
#define one_char_token(token, ttype) case token: \
            view_chopn(&l->text, 1);\
            l->col++; \
            tok->type = L_##ttype; \
            break;
        one_char_token('@', DOG)
        one_char_token('%', PERCENT)
        one_char_token('\\', BACKSLASH)
        one_char_token(';', SEMICOLON)
        one_char_token(')', RPAREN)
        one_char_token('(', LPAREN)
        one_char_token(']', RBRACK)
        one_char_token('[', LBRACK)
        one_char_token('~', TILDA)
        one_char_token('`', BACKTICK)
        one_char_token('}', RCURLY)
        one_char_token('{', LCURLY)
        one_char_token(':', COLON)
        one_char_token(',', COMMA)
        one_char_token('.', DOT)
        one_char_token('\'', QUOT)
#undef one_char_token
#define two_char_tokens(t1, t2, tt1, tt2) case t1: \
            view_chopn(&l->text, 1);\
            l->col++; \
            if (cur(l) == t2) { \
                view_chopn(&l->text, 1);\
                l->col++; \
                tok->type = tt2; \
            } else tok->type = tt1; \
            break;
        two_char_tokens('<', '=', L_LESS, L_EQ_LESS)
        two_char_tokens('>', '=', L_GREATER, L_EQ_GREATER)
        two_char_tokens('!', '=', L_BANG, L_BANG_EQ)
        two_char_tokens('=', '=', L_EQ, L_EQ2)
        two_char_tokens('?', '.', L_QUESTION, L_QDOT)
        two_char_tokens('*', '=', L_STAR, L_MULT_EQ)
        two_char_tokens('/', '=', L_DIV, L_DIV_EQ)
        two_char_tokens('^', '=', L_HAT, L_HAT_EQ)
#undef two_char_tokens
#define three_char_tokens(t1, t2, t3, tt1, tt2, tt3) \
        case t1: \
            view_chopn(&l->text, 1);\
            l->col++; \
            if (cur(l) == t2) { \
                view_chopn(&l->text, 1);\
                l->col++; \
                tok->type = tt2; \
            } else if (cur(l) == t3) { \
                view_chopn(&l->text, 1);\
                l->col++; \
                tok->type = tt3; \
            } else tok->type = tt1; \
            break;
        three_char_tokens('&', '&', '=', L_REF, L_BAND, L_BANG_EQ);
        three_char_tokens('|', '|', '=', L_PIPE, L_BOR, L_BOR_EQ);
        three_char_tokens('+', '+', '=', L_PLUS, L_PLUS2, L_PLUS_EQ);
#undef three_char_tokens
    case '-':
        view_chopn(&l->text, 1);
        l->col++;
        if (((l)->text.chars[0]) == '-') {
          view_chopn(&l->text, 1);
          l->col++;
          tok->type = L_MINUS2;
        } else if (((l)->text.chars[0]) == '>') {
          view_chopn(&l->text, 1);
          l->col++;
          tok->type = L_ARROW_R;
        } else if (((l)->text.chars[0]) == '=') {
          view_chopn(&l->text, 1);
          l->col++;
          tok->type = L_MINUS_EQ;
        } else tok->type = L_MINUS;
    break;
        default:
            lex_word(l, tok); break;
    };

    return tok;
};

token_t *next_token(struct lexer_t *l)
{
    if (l->toks.len != 0) {
        token_t *tok;
        tokens_remove(&l->toks, 0, &tok);
        return tok;
    } else return __next_token(l);
};

void push_token(struct lexer_t *l, token_t *tok)
{
    tokens_append(&l->toks, tok);
};

token_t *peek_token(struct lexer_t *l, size_t lookahead)
{
    while (l->toks.len < lookahead) push_token(l, __next_token(l));
    return l->toks.items[lookahead-1];
};

tokens *tokens_append(tokens *da, token_t *item) {
    if (da->len == da->cap) {
      da->items = realloc(da->items, da->cap * 2 * sizeof(token_t *));
      if (da->items == ((void *)0))
        return ((void *)0);
      da->cap *= 2;
    };
    da->items[da->len++] = item;
    return da;
};
token_t *tokens_get(tokens *da, size_t i) {
    if (i < da->len)
      return da->items[i];
    return (token_t *){0};
};
tokens *tokens_init(tokens *da, size_t i) {
    if ((da->items = malloc(i * sizeof(token_t *))) == ((void *)0))
      return ((void *)0);
    da->cap = i;
    da->len = 0;
    return da;
};

tokens *tokens_remove(tokens *da, size_t i, token_t **out) {
    if (i >= da->len) return ((void *)0);
    if (out != NULL) *out = da->items[i];

    for (size_t x = da->len--; i < x; i++) {
        da->items[i] = da->items[i + 1];
    };
    return da;
};
