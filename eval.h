#ifndef EVAL_H
#define EVAL_H

#include "ast.h"

/*
 * Explicit evaluation status codes.
 *
 * Using a named enum (rather than a plain int) makes call sites self-documenting
 * and allows future error categories to be added without breaking existing
 * comparisons (callers check `result.status == EVAL_OK`).
 */
typedef enum {
    EVAL_OK = 0,        /* successful evaluation                */
    EVAL_ERR_DIV_ZERO,  /* division by zero detected            */
    EVAL_ERR_OVERFLOW,  /* arithmetic overflow (future use)     */
    EVAL_ERR_INTERNAL   /* unexpected node type / NULL node     */
} EvalStatus;

/*
 * Evaluation result.
 *
 * Wrapping status + value in a struct avoids out-of-band sentinel values
 * and distinguishes a legitimate zero result from any error condition.
 */
typedef struct {
    long       value;
    EvalStatus status;
} EvalResult;

/*
 * Recursively evaluate the AST, printing a trace line for every binary
 * operation resolved.  Only `const Node *` is required — the evaluator
 * never mutates the tree.
 *
 * Trace format (to stdout):
 *   MUL 5 2 -> 10
 *
 * On error, a message is printed to stderr and result.status is non-zero.
 */
EvalResult eval(const Node *node);

#endif /* EVAL_H */
