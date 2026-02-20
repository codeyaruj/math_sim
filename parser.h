#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/*
 * Parser state.  Wraps the token stream and tracks whether a fatal error
 * has been encountered so that callers can unwind cleanly.
 */
typedef struct {
    TokenStream *ts;
    int          error; /* non-zero after the first parse error */
} Parser;

/* Initialise parser over an existing (already-initialised) token stream. */
void  parser_init(Parser *p, TokenStream *ts);

/*
 * Entry point.  Returns the root AST node, or NULL on error.
 * On success, the entire input must have been consumed (TOK_EOF).
 *
 * Grammar (classic additive/multiplicative precedence):
 *
 *   expr   → term   (('+' | '-') term)*
 *   term   → factor (('*' | '/') factor)*
 *   factor → NUMBER | '(' expr ')'
 */
Node *parser_parse(Parser *p);

/* Individual grammar productions — exposed for unit testing. */
Node *parse_expr(Parser *p);
Node *parse_term(Parser *p);
Node *parse_factor(Parser *p);

#endif /* PARSER_H */
