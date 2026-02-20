#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* ── Token types ──────────────────────────────────────────────────────────── */
typedef enum {
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    long      value;    /* valid when type == TOK_NUMBER */
    size_t    pos;      /* byte offset in source, for error messages */
} Token;

/* ── Token stream ─────────────────────────────────────────────────────────── */
/*
 * TokenStream owns no memory beyond the slice it points into.
 * Caller must keep the source string alive for the stream's lifetime.
 */
typedef struct {
    const char *src;    /* original source string (not owned) */
    size_t      len;    /* length of src */
    size_t      pos;    /* current read head */

    /* One-token look-ahead cache */
    Token       current;
    int         has_current; /* non-zero when current is valid */
} TokenStream;

/* Initialise a stream over a NUL-terminated source string. */
void  lexer_init(TokenStream *ts, const char *src);

/* Return the next token (advances the stream). */
Token lexer_next(TokenStream *ts);

/*
 * Peek at the next token WITHOUT consuming it.
 *
 * Contract:
 *  - Idempotent: calling lexer_peek() multiple times in a row returns the
 *    same token and does not advance the stream position.
 *  - The cached token is stored inside the TokenStream itself; no dynamic
 *    allocation occurs.
 *  - The cache is invalidated only by the next call to lexer_next().
 *  - It is safe to interleave lexer_peek() and lexer_next() calls freely.
 */
Token lexer_peek(TokenStream *ts);

/* Human-readable name for a token type (for error messages). */
const char *token_type_name(TokenType t);

#endif /* LEXER_H */
