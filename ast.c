#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

/* ── Constructors ─────────────────────────────────────────────────────────── */

Node *ast_make_number(long value)
{
    Node *n = malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }
    n->type  = NODE_NUMBER;
    n->value = value;
    return n;
}

Node *ast_make_binary(BinaryOp op, Node *left, Node *right)
{
    /* Defensive: a binary node with a NULL child is always a caller bug. */
    if (!left || !right) {
        fprintf(stderr, "ast error: ast_make_binary called with NULL child "
                        "(left=%p, right=%p)\n", (void *)left, (void *)right);
        ast_free(left);
        ast_free(right);
        exit(EXIT_FAILURE);
    }
    Node *n = malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }
    n->type         = NODE_BINARY_OP;
    n->binary.op    = op;
    n->binary.left  = left;
    n->binary.right = right;
    return n;
}

/* ── Destructor ───────────────────────────────────────────────────────────── */

void ast_free(Node *node)
{
    if (!node) return;
    if (node->type == NODE_BINARY_OP) {
        ast_free(node->binary.left);
        ast_free(node->binary.right);
    }
    free(node);
}

/* ── Debug dump ───────────────────────────────────────────────────────────── */

static const char *op_name(BinaryOp op)
{
    switch (op) {
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
    }
    return "???";
}

void ast_dump(const Node *node, int depth)
{
    if (!node) return;
    for (int i = 0; i < depth * 2; i++) fputc(' ', stderr);
    if (node->type == NODE_NUMBER) {
        fprintf(stderr, "NUMBER(%ld)\n", node->value);
    } else {
        fprintf(stderr, "%s\n", op_name(node->binary.op));
        ast_dump(node->binary.left,  depth + 1);
        ast_dump(node->binary.right, depth + 1);
    }
}
