#include "eval.h"

#include <stdio.h>

/* ── Internal helpers ─────────────────────────────────────────────────────── */

static EvalResult make_ok(long v)
{
    return (EvalResult){ .value = v, .status = EVAL_OK };
}

static EvalResult make_err(EvalStatus s)
{
    return (EvalResult){ .value = 0, .status = s };
}

static const char *op_label(BinaryOp op)
{
    switch (op) {
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
    }
    return "???";
}

/* ── Recursive evaluator ──────────────────────────────────────────────────── */

EvalResult eval(const Node *node)
{
    if (!node) {
        fprintf(stderr, "eval error: NULL node\n");
        return make_err(EVAL_ERR_INTERNAL);
    }

    switch (node->type) {

        case NODE_NUMBER:
            /* Leaf — no trace line; just return the value. */
            return make_ok(node->value);

        case NODE_BINARY_OP: {
            /*
             * Post-order traversal: evaluate children before the operator.
             * This ensures trace lines appear in evaluation order
             * (innermost sub-expressions first), matching how a real
             * interpreter would emit IR instructions.
             */
            EvalResult lhs = eval(node->binary.left);
            if (lhs.status != EVAL_OK) return make_err(lhs.status);

            EvalResult rhs = eval(node->binary.right);
            if (rhs.status != EVAL_OK) return make_err(rhs.status);

            long result;
            const char *label = op_label(node->binary.op);

            switch (node->binary.op) {
                case OP_ADD:
                    /* TODO(Level-2): check signed overflow before adding */
                    result = lhs.value + rhs.value;
                    break;
                case OP_SUB:
                    /* TODO(Level-2): check signed underflow before subtracting */
                    result = lhs.value - rhs.value;
                    break;
                case OP_MUL:
                    /* TODO(Level-2): check signed overflow before multiplying */
                    result = lhs.value * rhs.value;
                    break;
                case OP_DIV:
                    if (rhs.value == 0) {
                        fprintf(stderr, "eval error: division by zero\n");
                        return make_err(EVAL_ERR_DIV_ZERO);
                    }
                    result = lhs.value / rhs.value;
                    break;
                default:
                    fprintf(stderr, "eval error: unknown operator\n");
                    return make_err(EVAL_ERR_INTERNAL);
            }

            /* Emit one trace line per binary node resolved. */
            printf("%s %ld %ld -> %ld\n", label, lhs.value, rhs.value, result);
            return make_ok(result);
        }
    }

    fprintf(stderr, "eval error: unknown node type\n");
    return make_err(EVAL_ERR_INTERNAL);
}

