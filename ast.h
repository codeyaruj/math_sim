#ifndef AST_H
#define AST_H

/* AST node types — extensible: add NODE_UNARY_OP, NODE_CALL, etc. later */
typedef enum {
    NODE_NUMBER,
    NODE_BINARY_OP
} NodeType;

/* Operator tag stored in binary nodes */
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinaryOp;

typedef struct Node Node;

struct Node {
    NodeType type;
    union {
        /* NODE_NUMBER */
        long value;

        /* NODE_BINARY_OP */
        struct {
            BinaryOp op;
            Node    *left;
            Node    *right;
        } binary;
    };
};

/* Constructors */
Node *ast_make_number(long value);
Node *ast_make_binary(BinaryOp op, Node *left, Node *right);

/* Recursive destructor — frees the entire subtree */
void  ast_free(Node *node);

/* Debug dump (optional, useful during development) */
void  ast_dump(const Node *node, int depth);

#endif /* AST_H */
