#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "ir.h"

/*
 * Code generator: walks the AST and emits IR instructions.
 *
 * Register allocation strategy (Level-2: linear / no reuse):
 *   - Each AST sub-expression gets a fresh virtual register.
 *   - next_reg is a monotonically increasing counter.
 *   - Binary ops consume two child registers and produce a result in the
 *     left child's register (matching the IR binary-op semantics where
 *     dst is also an implicit lhs operand).
 *
 * This simple scheme is correct and easy to extend: a Level-3 pass could
 * inspect liveness and reuse dead registers without touching the grammar.
 */

typedef struct {
    IRProgram *prog;     /* output buffer (not owned — caller manages it) */
    int        next_reg; /* next free virtual register number             */
} Codegen;

/* Initialise a code generator that appends into `prog`. */
void codegen_init(Codegen *cg, IRProgram *prog);

/*
 * Recursively compile `node` into IR, appending to cg->prog.
 * Returns the register number that holds the result of this sub-expression.
 * Calls exit(EXIT_FAILURE) on internal errors (NULL node, unknown type).
 */
int codegen_expr(Codegen *cg, const Node *node);

#endif /* CODEGEN_H */
