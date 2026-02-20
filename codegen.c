#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>

/* ── Init ─────────────────────────────────────────────────────────────────── */

void codegen_init(Codegen *cg, IRProgram *prog)
{
    cg->prog     = prog;
    cg->next_reg = 0;
}

/* ── Register allocator ───────────────────────────────────────────────────── */

/* Return the next free register and advance the counter. */
static int alloc_reg(Codegen *cg)
{
    return cg->next_reg++;
}

/* ── Opcode mapping ───────────────────────────────────────────────────────── */

static IROpcode ast_op_to_ir(BinaryOp op)
{
    switch (op) {
        case OP_ADD: return IR_ADD;
        case OP_SUB: return IR_SUB;
        case OP_MUL: return IR_MUL;
        case OP_DIV: return IR_DIV;
    }
    /* Unreachable if the AST is well-formed. */
    fprintf(stderr, "codegen error: unknown BinaryOp %d\n", (int)op);
    exit(EXIT_FAILURE);
}

/* ── Core recursive walk ──────────────────────────────────────────────────── */

int codegen_expr(Codegen *cg, const Node *node)
{
    if (!node) {
        fprintf(stderr, "codegen error: NULL node\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type) {

        case NODE_NUMBER: {
            /*
             * Leaf: allocate a fresh register and load the constant.
             *
             *   LOAD_CONST  Rn, <value>
             */
            int reg = alloc_reg(cg);
            ir_program_append(cg->prog, (IRInstr){
                .op  = IR_LOAD_CONST,
                .dst = reg,
                .src = 0,   /* unused for LOAD_CONST */
                .imm = node->value
            });
            return reg;
        }

        case NODE_BINARY_OP: {
            /*
             * Binary node — post-order: compile children first so their
             * LOAD_CONST instructions appear before the operator that uses them.
             *
             * Steps:
             *   1. left_reg  = codegen_expr(left)
             *   2. right_reg = codegen_expr(right)
             *   3. emit: OP left_reg, right_reg   (dst=left_reg, src=right_reg)
             *   4. return left_reg  — it now holds the sub-expression result
             *
             * Using left_reg as both the lhs operand and the destination
             * mirrors real register-machine conventions (e.g. x86 two-address).
             */
            int left_reg  = codegen_expr(cg, node->binary.left);
            int right_reg = codegen_expr(cg, node->binary.right);

            ir_program_append(cg->prog, (IRInstr){
                .op  = ast_op_to_ir(node->binary.op),
                .dst = left_reg,
                .src = right_reg,
                .imm = 0    /* unused for binary ops */
            });

            return left_reg;
        }
    }

    fprintf(stderr, "codegen error: unknown node type %d\n", (int)node->type);
    exit(EXIT_FAILURE);
}
