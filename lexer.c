#include "lexer.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal helpers ─────────────────────────────────────────────────────── */

static Token make_token(TokenType type, long value, size_t pos)
{
    return (Token){ .type = type, .value = value, .pos = pos };
}

/* Skip whitespace; return updated position. */
static size_t skip_ws(const char *src, size_t pos, size_t len)
{
    while (pos < len && isspace((unsigned char)src[pos]))
        pos++;
    return pos;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void lexer_init(TokenStream *ts, const char *src)
{
    ts->src         = src;
    ts->len         = strlen(src);
    ts->pos         = 0;
    ts->has_current = 0;
}

/*
 * Scan and return the next token.
 * Single-pass O(n) — each character is visited at most once.
 */
Token lexer_next(TokenStream *ts)
{
    /* Drain the look-ahead cache first. */
    if (ts->has_current) {
        ts->has_current = 0;
        return ts->current;
    }

    ts->pos = skip_ws(ts->src, ts->pos, ts->len);

    if (ts->pos >= ts->len)
        return make_token(TOK_EOF, 0, ts->pos);

    size_t     start = ts->pos;
    const char c     = ts->src[ts->pos];

    /* Multi-digit integer literal — overflow-checked. */
    if (isdigit((unsigned char)c)) {
        long value = 0;
        while (ts->pos < ts->len && isdigit((unsigned char)ts->src[ts->pos])) {
            int digit = ts->src[ts->pos] - '0';
            /*
             * Overflow check: value * 10 + digit > LONG_MAX
             * Rearranged to avoid triggering UB:
             *   value > (LONG_MAX - digit) / 10
             */
            if (value > (LONG_MAX - digit) / 10) {
                fprintf(stderr, "lexer error: integer overflow at position %zu\n", start);
                /* Drain remaining digits so the stream stays consistent. */
                while (ts->pos < ts->len && isdigit((unsigned char)ts->src[ts->pos]))
                    ts->pos++;
                return make_token(TOK_INVALID, 0, start);
            }
            value = value * 10 + digit;
            ts->pos++;
        }
        return make_token(TOK_NUMBER, value, start);
    }

    ts->pos++; /* consume single-character token */

    switch (c) {
        case '+': return make_token(TOK_PLUS,   0, start);
        case '-': return make_token(TOK_MINUS,  0, start);
        case '*': return make_token(TOK_MUL,    0, start);
        case '/': return make_token(TOK_DIV,    0, start);
        case '(': return make_token(TOK_LPAREN, 0, start);
        case ')': return make_token(TOK_RPAREN, 0, start);
        default:
            fprintf(stderr, "lexer error: invalid character '%c' at position %zu\n",
                    c, start);
            return make_token(TOK_INVALID, 0, start);
    }
}

Token lexer_peek(TokenStream *ts)
{
    if (!ts->has_current) {
        ts->current     = lexer_next(ts);
        ts->has_current = 1;
    }
    return ts->current;
}

const char *token_type_name(TokenType t)
{
    switch (t) {
        case TOK_NUMBER:  return "NUMBER";
        case TOK_PLUS:    return "+";
        case TOK_MINUS:   return "-";
        case TOK_MUL:     return "*";
        case TOK_DIV:     return "/";
        case TOK_LPAREN:  return "(";
        case TOK_RPAREN:  return ")";
        case TOK_EOF:     return "EOF";
        case TOK_INVALID: return "INVALID";
    }
    return "UNKNOWN";
}
