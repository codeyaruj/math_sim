#ifndef CPU_H
#define CPU_H

#include "ir.h"
#include "alu.h"
#include "memory.h"

/*
 * Virtual CPU — Level-5 load/store architecture.
 *
 * Changes from Level-4:
 *   - CPU holds a pointer to Memory (not owned — caller allocates/frees it).
 *     This is the standard approach on real hardware: the CPU and RAM are
 *     separate chips connected by a bus.  Keeping them separate here means
 *     a future cache layer can intercept between the two without changing
 *     the CPU source.
 *   - New instructions: IR_LOAD, IR_STORE.
 *   - cpu_execute now takes a `Memory *mem` parameter.
 *
 * Invariants preserved from Level-4:
 *   - Register file is `word_t` (uint32_t).
 *   - All arithmetic still flows through the ALU.
 *   - PC-driven fetch-decode-execute loop.
 *   - `flags` reflects the most recent ALU-touching operation.
 *   - LOAD, STORE, and jump instructions do NOT modify flags.
 */

#define CPU_MAX_REGS  32
#define CPU_MAX_STEPS 1000000   /* infinite-loop guard */

typedef struct {
    word_t   regs[CPU_MAX_REGS]; /* 32-bit register file          */
    ALUFlags flags;              /* flags from last ALU operation  */
    size_t   pc;                 /* program counter               */
    Memory  *mem;                /* RAM — not owned by CPU        */
} CPU;

/*
 * Execute `prog` on a freshly zeroed CPU backed by `mem`.
 *
 * `mem` may be NULL if the program contains no LOAD/STORE instructions;
 * a NULL mem with a LOAD/STORE will produce a cpu error at runtime.
 *
 * Prints a trace line per instruction.
 * Stores sign-extended result of the last-written register in *out_result.
 * Returns 0 on success, -1 on error.
 */
int cpu_execute(const IRProgram *prog, Memory *mem, long *out_result);

#endif /* CPU_H */



