#ifndef IR_H
#define IR_H

#include <stddef.h>

/*
 * IR — Instruction Representation layer
 *
 * This sits between the AST (source-level tree) and the CPU (executor).
 * Keeping it explicit means we can later add optimization passes between
 * codegen and execution without touching either the parser or the CPU.
 *
 * Register-machine model:
 *   - Virtual registers are plain integers (unlimited in IR; CPU caps at 32).
 *   - All values are `long` (matching the AST / evaluator).
 *
 * Level-4 additions: CMP, JMP, JZ, JNZ.
 * The `target` field carries the destination program-counter index for
 * jump instructions.  It is ignored (and should be set to 0) for all
 * non-branch instructions so that the struct stays zero-initializable.
 */

/* ── Opcode set ───────────────────────────────────────────────────────────── */

typedef enum {
    /* ── Level-2: arithmetic ──────────────────────────────────────────────── */
    IR_LOAD_CONST, /* R[dst] = imm                                            */
    IR_ADD,        /* R[dst] = R[dst] + R[src]                                */
    IR_SUB,        /* R[dst] = R[dst] - R[src]                                */
    IR_MUL,        /* R[dst] = R[dst] * R[src]                                */
    IR_DIV,        /* R[dst] = R[dst] / R[src]                                */

    /* ── Level-4: comparison & control flow ──────────────────────────────── */
    IR_CMP,        /* flags = R[dst] - R[src]  (result discarded)             */
    IR_JMP,        /* PC = target  (unconditional)                            */
    IR_JZ,         /* if (Z==1) PC = target                                   */
    IR_JNZ,        /* if (Z==0) PC = target                                   */

    /* ── Level-5: memory access ──────────────────────────────────────────── */
    IR_LOAD,       /* R[dst] = MEM[R[addr]]    (32-bit word load)             */
    IR_STORE       /* MEM[R[addr]] = R[src]    (32-bit word store)            */
} IROpcode;

/* ── Single instruction ───────────────────────────────────────────────────── */

typedef struct {
    IROpcode op;
    int      dst;    /* destination register (arithmetic / LOAD)              */
    int      src;    /* source register      (arithmetic / CMP / STORE)       */
    long     imm;    /* immediate value      (LOAD_CONST only)                */
    int      target; /* jump destination PC  (JMP/JZ/JNZ only)               */
    int      addr;   /* register holding memory address (LOAD/STORE only)     */
} IRInstr;

/* ── Dynamic instruction buffer ──────────────────────────────────────────── */

typedef struct {
    IRInstr *data;
    size_t   count;
    size_t   capacity;
} IRProgram;

/* Lifecycle */
void ir_program_init(IRProgram *prog);
void ir_program_free(IRProgram *prog);

/*
 * Append one instruction to the program, growing the buffer as needed.
 * Terminates on allocation failure (unrecoverable).
 */
void ir_program_append(IRProgram *prog, IRInstr instr);

/* Debug: dump all instructions to stderr. */
void ir_program_dump(const IRProgram *prog);

/* Human-readable opcode name (for traces and dumps). */
const char *ir_opcode_name(IROpcode op);

#endif /* IR_H */

