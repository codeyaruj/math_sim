#include "cpu.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Flags string buffer: "Z=0 N=0 C=0 V=0" + NUL */
#define FLAGS_BUF 24

/* ── Internal validation ──────────────────────────────────────────────────── */

static int check_reg(int r, const char *role, size_t pc)
{
    if (r < 0 || r >= CPU_MAX_REGS) {
        fprintf(stderr, "cpu error: %s register R%d out of range "
                        "(max R%d) at pc=%zu\n",
                role, r, CPU_MAX_REGS - 1, pc);
        return -1;
    }
    return 0;
}

/*
 * Validate a jump target.
 * target in [0, prog_count] is valid:
 *   - [0, prog_count-1]  jumps to an instruction
 *   - prog_count         jumps past the last instruction → loop exits (halt)
 * target outside this range is a bug.
 */
static int check_target(int target, size_t prog_count, size_t pc)
{
    if (target < 0 || (size_t)target > prog_count) {
        fprintf(stderr, "cpu error: jump target %d out of bounds "
                        "(program has %zu instructions) at pc=%zu\n",
                target, prog_count, pc);
        return -1;
    }
    return 0;
}

/* ── PC-driven execution loop ─────────────────────────────────────────────── */

int cpu_execute(const IRProgram *prog, Memory *mem, long *out_result)
{
    if (!prog || prog->count == 0) {
        fprintf(stderr, "cpu error: empty program\n");
        return -1;
    }

    CPU cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.mem = mem;   /* may be NULL for arithmetic-only programs */

    char   fbuf[FLAGS_BUF];
    int    last_dst   = 0;
    size_t step_count = 0;  /* total instructions dispatched (loop guard) */

    /*
     * PC-driven fetch-decode-execute loop.
     *
     * Each iteration:
     *   1. Bounds-check pc.
     *   2. Fetch instruction at prog->data[pc].
     *   3. Execute: arithmetic increments pc after; jumps set pc directly.
     *
     * `jumped` is set to 1 when an instruction has already written pc;
     * the post-switch increment is skipped in that case.
     */
    while (cpu.pc < prog->count) {

        if (++step_count > CPU_MAX_STEPS) {
            fprintf(stderr, "cpu error: execution limit (%d steps) exceeded "
                            "— possible infinite loop at pc=%zu\n",
                    CPU_MAX_STEPS, cpu.pc);
            return -1;
        }

        const IRInstr *in     = &prog->data[cpu.pc];
        int            jumped = 0;  /* set to 1 if this instruction wrote pc */

        switch (in->op) {

            /* ── LOAD_CONST ──────────────────────────────────────────────── */
            case IR_LOAD_CONST: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                cpu.regs[in->dst] = (word_t)(uint32_t)in->imm;
                /* LOAD_CONST does NOT modify flags. */
                printf("[CPU pc=%zu] R%d = %u\n",
                       cpu.pc, in->dst, (unsigned)cpu.regs[in->dst]);
                last_dst = in->dst;
                break;
            }

            /* ── ADD ─────────────────────────────────────────────────────── */
            case IR_ADD: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                if (check_reg(in->src, "src", cpu.pc) != 0) return -1;
                word_t res = alu_add(cpu.regs[in->dst], cpu.regs[in->src],
                                     &cpu.flags);
                cpu.regs[in->dst] = res;
                alu_flags_str(&cpu.flags, fbuf, FLAGS_BUF);
                printf("[CPU pc=%zu] R%d = R%d + R%d -> %u  (%s)\n",
                       cpu.pc, in->dst, in->dst, in->src,
                       (unsigned)res, fbuf);
                last_dst = in->dst;
                break;
            }

            /* ── SUB ─────────────────────────────────────────────────────── */
            case IR_SUB: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                if (check_reg(in->src, "src", cpu.pc) != 0) return -1;
                word_t res = alu_sub(cpu.regs[in->dst], cpu.regs[in->src],
                                     &cpu.flags);
                cpu.regs[in->dst] = res;
                alu_flags_str(&cpu.flags, fbuf, FLAGS_BUF);
                printf("[CPU pc=%zu] R%d = R%d - R%d -> %u  (%s)\n",
                       cpu.pc, in->dst, in->dst, in->src,
                       (unsigned)res, fbuf);
                last_dst = in->dst;
                break;
            }

            /* ── MUL ─────────────────────────────────────────────────────── */
            case IR_MUL: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                if (check_reg(in->src, "src", cpu.pc) != 0) return -1;
                word_t res = alu_mul(cpu.regs[in->dst], cpu.regs[in->src],
                                     &cpu.flags);
                cpu.regs[in->dst] = res;
                alu_flags_str(&cpu.flags, fbuf, FLAGS_BUF);
                printf("[CPU pc=%zu] R%d = R%d * R%d -> %u  (%s)\n",
                       cpu.pc, in->dst, in->dst, in->src,
                       (unsigned)res, fbuf);
                last_dst = in->dst;
                break;
            }

            /* ── DIV ─────────────────────────────────────────────────────── */
            case IR_DIV: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                if (check_reg(in->src, "src", cpu.pc) != 0) return -1;
                if (cpu.regs[in->src] == 0u) {
                    fprintf(stderr,
                            "cpu error: division by zero (R%d = 0) at pc=%zu\n",
                            in->src, cpu.pc);
                    return -1;
                }
                word_t res = alu_div(cpu.regs[in->dst], cpu.regs[in->src],
                                     &cpu.flags);
                cpu.regs[in->dst] = res;
                alu_flags_str(&cpu.flags, fbuf, FLAGS_BUF);
                printf("[CPU pc=%zu] R%d = R%d / R%d -> %u  (%s)\n",
                       cpu.pc, in->dst, in->dst, in->src,
                       (unsigned)res, fbuf);
                last_dst = in->dst;
                break;
            }

            /* ── CMP ─────────────────────────────────────────────────────── */
            /*
             * Subtract src from dst via the ALU, update flags, discard result.
             * This is identical to SUB except the destination register is NOT
             * written — exactly how ARM's CMP instruction works.
             * CMP does NOT update last_dst (no register is written).
             */
            case IR_CMP: {
                if (check_reg(in->dst, "dst", cpu.pc) != 0) return -1;
                if (check_reg(in->src, "src", cpu.pc) != 0) return -1;
                alu_sub(cpu.regs[in->dst], cpu.regs[in->src], &cpu.flags);
                alu_flags_str(&cpu.flags, fbuf, FLAGS_BUF);
                printf("[CPU pc=%zu] CMP R%d, R%d  (%s)\n",
                       cpu.pc, in->dst, in->src, fbuf);
                /* flags updated; no register written */
                break;
            }

            /* ── JMP ─────────────────────────────────────────────────────── */
            case IR_JMP: {
                if (check_target(in->target, prog->count, cpu.pc) != 0)
                    return -1;
                printf("[CPU pc=%zu] JMP -> target=%d\n",
                       cpu.pc, in->target);
                cpu.pc = (size_t)in->target;
                jumped = 1;
                /* JMP does NOT modify flags or registers */
                break;
            }

            /* ── JZ ──────────────────────────────────────────────────────── */
            case IR_JZ: {
                if (cpu.flags.Z) {
                    if (check_target(in->target, prog->count, cpu.pc) != 0)
                        return -1;
                    printf("[CPU pc=%zu] JZ -> taken (target=%d)\n",
                           cpu.pc, in->target);
                    cpu.pc = (size_t)in->target;
                    jumped = 1;
                } else {
                    printf("[CPU pc=%zu] JZ -> not taken\n", cpu.pc);
                }
                break;
            }

            /* ── JNZ ─────────────────────────────────────────────────────── */
            case IR_JNZ: {
                if (!cpu.flags.Z) {
                    if (check_target(in->target, prog->count, cpu.pc) != 0)
                        return -1;
                    printf("[CPU pc=%zu] JNZ -> taken (target=%d)\n",
                           cpu.pc, in->target);
                    cpu.pc = (size_t)in->target;
                    jumped = 1;
                } else {
                    printf("[CPU pc=%zu] JNZ -> not taken\n", cpu.pc);
                }
                break;
            }

            /* ── LOAD ───────────────────────────────────────────────────── */
            /*
             * R[dst] = MEM[R[addr]]
             * The address register holds a byte address; access must be
             * 32-bit aligned.  Flags are NOT modified.
             */
            case IR_LOAD: {
                if (check_reg(in->dst,  "dst",  cpu.pc) != 0) return -1;
                if (check_reg(in->addr, "addr", cpu.pc) != 0) return -1;
                if (!cpu.mem) {
                    fprintf(stderr, "cpu error: LOAD at pc=%zu but no memory "
                                    "was attached to this CPU\n", cpu.pc);
                    return -1;
                }
                uint32_t addr  = cpu.regs[in->addr];
                uint32_t value = 0;
                if (mem_read_word(cpu.mem, addr, &value) != 0) return -1;
                cpu.regs[in->dst] = (word_t)value;
                printf("[CPU pc=%zu] LOAD R%d <- MEM[0x%04x] -> %u\n",
                       cpu.pc, in->dst, (unsigned)addr, (unsigned)value);
                last_dst = in->dst;
                break;
            }

            /* ── STORE ──────────────────────────────────────────────────── */
            /*
             * MEM[R[addr]] = R[src]
             * Flags are NOT modified.
             */
            case IR_STORE: {
                if (check_reg(in->src,  "src",  cpu.pc) != 0) return -1;
                if (check_reg(in->addr, "addr", cpu.pc) != 0) return -1;
                if (!cpu.mem) {
                    fprintf(stderr, "cpu error: STORE at pc=%zu but no memory "
                                    "was attached to this CPU\n", cpu.pc);
                    return -1;
                }
                uint32_t addr  = cpu.regs[in->addr];
                uint32_t value = cpu.regs[in->src];
                if (mem_write_word(cpu.mem, addr, value) != 0) return -1;
                printf("[CPU pc=%zu] STORE MEM[0x%04x] <- R%d (%u)\n",
                       cpu.pc, (unsigned)addr, in->src, (unsigned)value);
                /* STORE writes no register; last_dst unchanged */
                break;
            }

            default:
                fprintf(stderr, "cpu error: unknown opcode %d at pc=%zu\n",
                        (int)in->op, cpu.pc);
                return -1;
        }

        /* Advance PC unless a jump already set it. */
        if (!jumped)
            cpu.pc++;
    }

    if (out_result)
        *out_result = (long)(int32_t)cpu.regs[last_dst];

    return 0;
}


