#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

/* ── Internal error helper ────────────────────────────────────────────────── */

/*
 * Record a parse error.  We print immediately so the position is accurate,
 * set the error flag, and return so callers can propagate NULL upward.
 */
static void parse_error(Parser *p, const char *msg, Token t)
{
    fprintf(stderr, "parser error: at position %zu (token '%s'): %s\n",
            t.pos, token_type_name(t.type), msg);
    p->error = 1;
}

/*
 * Consume the next token and verify it matches the expected type.
 * Returns the consumed token, or a synthetic TOK_INVALID on mismatch.
 */
static Token expect(Parser *p, TokenType expected)
{
    Token t = lexer_next(p->ts);
    if (t.type != expected) {
        char buf[128];
        snprintf(buf, sizeof(buf), "expected '%s' but got '%s'",
                 token_type_name(expected), token_type_name(t.type));
        parse_error(p, buf, t);
    }
    return t;
}

/* ── Grammar productions ──────────────────────────────────────────────────── */

/*
 * factor → NUMBER | '(' expr ')'
 *
 * Lowest-level production; handles atoms and grouping.
 *
 * NOTE (future Level-2 extension): unary operators can be added here:
 *   factor → ('+' | '-') factor | NUMBER | '(' expr ')'
 * This would require a NODE_UNARY_OP AST node and a corresponding eval case.
 */
Node *parse_factor(Parser *p)
{
    if (p->error) return NULL;

    Token t = lexer_peek(p->ts);

    if (t.type == TOK_NUMBER) {
        lexer_next(p->ts); /* consume */
        return ast_make_number(t.value);
    }

    if (t.type == TOK_LPAREN) {
        lexer_next(p->ts); /* consume '(' */
        Node *inner = parse_expr(p);
        if (p->error) { ast_free(inner); return NULL; }
        expect(p, TOK_RPAREN);
        if (p->error) { ast_free(inner); return NULL; }
        return inner;
    }

    /* Anything else is a syntax error. */
    lexer_next(p->ts); /* consume so we have a real token for the message */
    parse_error(p, "expected a number or '('", t);
    return NULL;
}

/*
 * term → factor (('*' | '/') factor)*
 *
 * Handles multiplication and division (higher precedence than +/-).
 * Left-associative via the iterative loop.
 */
Node *parse_term(Parser *p)
{
    if (p->error) return NULL;

    Node *left = parse_factor(p);
    if (p->error) { ast_free(left); return NULL; }

    for (;;) {
        Token t = lexer_peek(p->ts);
        if (t.type != TOK_MUL && t.type != TOK_DIV)
            break;

        lexer_next(p->ts); /* consume operator */
        BinaryOp op = (t.type == TOK_MUL) ? OP_MUL : OP_DIV;

        Node *right = parse_factor(p);
        if (p->error) { ast_free(left); ast_free(right); return NULL; }

        left = ast_make_binary(op, left, right);
    }

    return left;
}

/*
 * expr → term (('+' | '-') term)*
 *
 * Handles addition and subtraction (lowest precedence).
 * Left-associative via the iterative loop.
 */
Node *parse_expr(Parser *p)
{
    if (p->error) return NULL;

    Node *left = parse_term(p);
    if (p->error) { ast_free(left); return NULL; }

    for (;;) {
        Token t = lexer_peek(p->ts);
        if (t.type != TOK_PLUS && t.type != TOK_MINUS)
            break;

        lexer_next(p->ts); /* consume operator */
        BinaryOp op = (t.type == TOK_PLUS) ? OP_ADD : OP_SUB;

        Node *right = parse_term(p);
        if (p->error) { ast_free(left); ast_free(right); return NULL; }

        left = ast_make_binary(op, left, right);
    }

    return left;
}

/* ── Public entry point ───────────────────────────────────────────────────── */

void parser_init(Parser *p, TokenStream *ts)
{
    p->ts    = ts;
    p->error = 0;
}

Node *parser_parse(Parser *p)
{
    Node *root = parse_expr(p);
    if (p->error) { ast_free(root); return NULL; }

    /* After a valid expression the very next token must be EOF. */
    Token t = lexer_peek(p->ts);
    if (t.type != TOK_EOF) {
        parse_error(p, "unexpected token after expression", t);
        ast_free(root);
        return NULL;
    }

    return root;
}
