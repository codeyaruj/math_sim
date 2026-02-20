/*
 * main.c — orchestration only.
 *
 * Full pipeline:
 *   stdin → lexer → parser → AST → (evaluator trace) → codegen → IR → CPU
 *
 * Level-1: recursive evaluator trace
 * Level-2: AST → IR compilation and virtual CPU execution
 * Level-3: bit-accurate ALU, 32-bit register file, CPU flags
 * Level-4: PC, CMP, conditional and unconditional jumps
 *
 * After the expression pipeline, a hand-written IR program demonstrates
 * the new control-flow instructions.
 */

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "eval.h"
#include "ir.h"
#include "codegen.h"
#include "cpu.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 4096

/* ── Level-4 demo: hand-written IR with branches ─────────────────────────── */
/*
 * Implements the pseudo-program:
 *
 *   R1 = 3
 *   R2 = 3
 *   CMP R1, R2          ; sets Z=1 because 3-3=0
 *   JZ  label_equal     ; taken → skip the "not equal" path
 *   R3 = 99             ; skipped
 *   JMP label_done      ; skipped
 * label_equal:           ; pc=6
 *   R3 = 42             ; executed
 * label_done:            ; pc=7  (program ends)
 *
 * Expected final state: R3 = 42.
 *
 * A second run uses R1=3, R2=5 to verify the not-taken path:
 * Expected final state: R3 = 99.
 */
static void run_branch_demo(void)
{
    printf("\n══════════════════════════════════════════\n");
    printf(" Level-4 branch demo — equal path (R1=3, R2=3)\n");
    printf("══════════════════════════════════════════\n");
    {
        /*
         * pc  instruction
         *  0  LOAD_CONST R1, 3
         *  1  LOAD_CONST R2, 3
         *  2  CMP        R1, R2
         *  3  JZ         6          (jump to label_equal)
         *  4  LOAD_CONST R3, 99
         *  5  JMP        7          (jump to label_done)
         *  6  LOAD_CONST R3, 42     (label_equal)
         *  7  <end of program>
         */
        IRProgram prog;
        ir_program_init(&prog);

        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=1,.imm=3  }); /* 0 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=2,.imm=3  }); /* 1 */
        ir_program_append(&prog, (IRInstr){.op=IR_CMP,       .dst=1,.src=2  }); /* 2 */
        ir_program_append(&prog, (IRInstr){.op=IR_JZ,        .target=6      }); /* 3 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=3,.imm=99 }); /* 4 */
        ir_program_append(&prog, (IRInstr){.op=IR_JMP,       .target=7      }); /* 5 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=3,.imm=42 }); /* 6 label_equal */

        long result = 0;
        int  status = cpu_execute(&prog, NULL, &result);
        ir_program_free(&prog);

        if (status == 0)
            printf("Branch demo result: R3 = %ld  (expected 42)\n", result);
        else
            fprintf(stderr, "Branch demo failed.\n");
    }

    printf("\n══════════════════════════════════════════\n");
    printf(" Level-4 branch demo — not-equal path (R1=3, R2=5)\n");
    printf("══════════════════════════════════════════\n");
    {
        /*
         * Same program structure, but R2=5 so CMP sets Z=0.
         * JZ is NOT taken; R3=99 is executed; JMP skips the equal path.
         */
        IRProgram prog;
        ir_program_init(&prog);

        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=1,.imm=3  }); /* 0 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=2,.imm=5  }); /* 1 */
        ir_program_append(&prog, (IRInstr){.op=IR_CMP,       .dst=1,.src=2  }); /* 2 */
        ir_program_append(&prog, (IRInstr){.op=IR_JZ,        .target=6      }); /* 3 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=3,.imm=99 }); /* 4 */
        ir_program_append(&prog, (IRInstr){.op=IR_JMP,       .target=7      }); /* 5 */
        ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=3,.imm=42 }); /* 6 label_equal */

        long result = 0;
        int  status = cpu_execute(&prog, NULL, &result);
        ir_program_free(&prog);

        if (status == 0)
            printf("Branch demo result: R3 = %ld  (expected 99)\n", result);
        else
            fprintf(stderr, "Branch demo failed.\n");
    }
}

/* ── Countdown loop demo ──────────────────────────────────────────────────── */
/*
 * Demonstrates JNZ with a simple countdown:
 *
 *   R0 = 5
 *   R1 = 1
 * loop:                  ; pc=2
 *   R0 = R0 - R1         ; R0--
 *   CMP R0, R0_zero      ; compare with zero sentinel isn't needed —
 *                        ; SUB already sets Z when result==0
 *   JNZ loop             ; if R0 != 0, loop
 *   <end>
 *
 * Simplified: SUB sets Z=1 when result is 0, so we can branch directly
 * on SUB's flags without a separate CMP.
 *
 *   R0 = 5
 *   R1 = 1
 * loop (pc=2):
 *   SUB R0, R1      ; R0 -= 1, sets Z when R0 reaches 0
 *   JNZ 2           ; loop while Z==0
 *
 * Expected: loop runs 5 times, final R0 = 0.
 */
static void run_loop_demo(void)
{
    printf("\n══════════════════════════════════════════\n");
    printf(" Level-4 loop demo — countdown R0 from 5 to 0\n");
    printf("══════════════════════════════════════════\n");

    IRProgram prog;
    ir_program_init(&prog);

    ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=0,.imm=5}); /* 0 */
    ir_program_append(&prog, (IRInstr){.op=IR_LOAD_CONST,.dst=1,.imm=1}); /* 1 */
    ir_program_append(&prog, (IRInstr){.op=IR_SUB,       .dst=0,.src=1}); /* 2  loop: */
    ir_program_append(&prog, (IRInstr){.op=IR_JNZ,       .target=2    }); /* 3  → back to SUB */

    long result = 0;
    int  status = cpu_execute(&prog, NULL, &result);
    ir_program_free(&prog);

    if (status == 0)
        printf("Loop demo result: R0 = %ld  (expected 0)\n", result);
    else
        fprintf(stderr, "Loop demo failed.\n");
}

/* ── Level-5 demo: load/store with RAM ───────────────────────────────────── */
/*
 * Demonstrates basic load/store:
 *
 *   R1 = 0x100          ; address
 *   R2 = 42             ; value to store
 *   STORE R2, [R1]      ; MEM[0x100] = 42
 *   LOAD  R3, [R1]      ; R3 = MEM[0x100]
 *
 * Expected: R3 == 42.
 *
 * Second run: stores 0xDEADBEEF, reloads it, verifies round-trip at the
 * 32-bit level.
 */
static void run_memory_demo(void)
{
    printf("\n══════════════════════════════════════════\n");
    printf(" Level-5 memory demo — basic store/load (value=42)\n");
    printf("══════════════════════════════════════════\n");
    {
        /*
         * pc  instruction
         *  0  LOAD_CONST R1, 0x100        address = 256
         *  1  LOAD_CONST R2, 42           value
         *  2  STORE      R2, [R1]         MEM[256] = 42
         *  3  LOAD       R3, [R1]         R3 = MEM[256]
         */
        IRProgram prog;
        ir_program_init(&prog);

        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 1, 0, 0x100, 0, 0}); /* 0 */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 2, 0, 42,    0, 0}); /* 1 */
        ir_program_append(&prog,
            (IRInstr){IR_STORE, 0, 2, 0, 0, 1});           /* 2  src=R2, addr=R1 */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD,  3, 0, 0, 0, 1});           /* 3  dst=R3, addr=R1 */

        Memory mem;
        mem_init(&mem);

        long result = 0;
        int  status = cpu_execute(&prog, &mem, &result);
        ir_program_free(&prog);

        if (status == 0)
            printf("Memory demo result: R3 = %ld  (expected 42)\n", result);
        else
            fprintf(stderr, "Memory demo failed.\n");
    }

    printf("\n══════════════════════════════════════════\n");
    printf(" Level-5 memory demo — round-trip 0xDEADBEEF\n");
    printf("══════════════════════════════════════════\n");
    {
        IRProgram prog;
        ir_program_init(&prog);

        /* Store 0xDEADBEEF at address 0x200, reload into R3. */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 0, 0, 0x200,       0, 0}); /* addr */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 1, 0, 0xDEADBEEF,  0, 0}); /* val  */
        ir_program_append(&prog,
            (IRInstr){IR_STORE, 0, 1, 0, 0, 0});                 /* MEM[R0]=R1 */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD,  2, 0, 0, 0, 0});                 /* R2=MEM[R0] */

        Memory mem;
        mem_init(&mem);

        long result = 0;
        int  status = cpu_execute(&prog, &mem, &result);
        ir_program_free(&prog);

        if (status == 0)
            printf("Round-trip result: R2 = 0x%08lx  (expected 0xdeadbeef)\n",
                   (unsigned long)(uint32_t)result);
        else
            fprintf(stderr, "Round-trip demo failed.\n");
    }

    printf("\n══════════════════════════════════════════\n");
    printf(" Level-5 error demo — unaligned access (0x102)\n");
    printf("══════════════════════════════════════════\n");
    {
        IRProgram prog;
        ir_program_init(&prog);

        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 0, 0, 0x102, 0, 0}); /* unaligned addr */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 1, 0, 7,     0, 0});
        ir_program_append(&prog,
            (IRInstr){IR_STORE, 0, 1, 0, 0, 0});           /* should fail */

        Memory mem;
        mem_init(&mem);

        long result = 0;
        int  status = cpu_execute(&prog, &mem, &result);
        ir_program_free(&prog);

        printf("Unaligned store returned: %s  (expected: error)\n",
               status != 0 ? "error (correct)" : "success (WRONG!)");
    }

    printf("\n══════════════════════════════════════════\n");
    printf(" Level-5 error demo — out-of-bounds access\n");
    printf("══════════════════════════════════════════\n");
    {
        IRProgram prog;
        ir_program_init(&prog);

        /* Address 0x10000 == MEM_SIZE; one word past end. */
        ir_program_append(&prog,
            (IRInstr){IR_LOAD_CONST, 0, 0, 0x10000, 0, 0});
        ir_program_append(&prog,
            (IRInstr){IR_LOAD, 1, 0, 0, 0, 0});             /* should fail */

        Memory mem;
        mem_init(&mem);

        long result = 0;
        int  status = cpu_execute(&prog, &mem, &result);
        ir_program_free(&prog);

        printf("Out-of-bounds load returned: %s  (expected: error)\n",
               status != 0 ? "error (correct)" : "success (WRONG!)");
    }
}



int main(void)
{
    /* ── 1. Read one line from stdin ──────────────────────────────────────── */
    char buf[MAX_INPUT];
    if (!fgets(buf, sizeof(buf), stdin)) {
        fprintf(stderr, "error: failed to read input\n");
        return EXIT_FAILURE;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[--len] = '\0';

    if (len == 0) {
        fprintf(stderr, "error: empty input\n");
        return EXIT_FAILURE;
    }

    /* ── 2. Lex ───────────────────────────────────────────────────────────── */
    TokenStream ts;
    lexer_init(&ts, buf);

    {
        TokenStream probe = ts;
        for (;;) {
            Token t = lexer_next(&probe);
            if (t.type == TOK_EOF)     break;
            if (t.type == TOK_INVALID) return EXIT_FAILURE;
        }
    }

    /* ── 3. Parse ─────────────────────────────────────────────────────────── */
    Parser parser;
    parser_init(&parser, &ts);

    Node *root = parser_parse(&parser);
    if (!root || parser.error) {
        ast_free(root);
        return EXIT_FAILURE;
    }

    /* ── 4. Level-1: recursive evaluator trace ────────────────────────────── */
    printf("TRACE:\n");
    EvalResult eval_result = eval(root);
    if (eval_result.status != EVAL_OK) {
        ast_free(root);
        return EXIT_FAILURE;
    }

    /* ── 5. Level-2/3/4: compile AST → IR → execute on CPU ───────────────── */
    IRProgram prog;
    ir_program_init(&prog);

    Codegen cg;
    codegen_init(&cg, &prog);
    codegen_expr(&cg, root);

    ast_free(root);

    printf("\nCPU:\n");
    long cpu_result = 0;
    int  cpu_status = cpu_execute(&prog, NULL, &cpu_result);

    ir_program_free(&prog);

    if (cpu_status != 0)
        return EXIT_FAILURE;

    /* ── 6. Cross-check at 32-bit level ──────────────────────────────────── */
    if ((uint32_t)cpu_result != (uint32_t)eval_result.value) {
        fprintf(stderr, "error: evaluator (0x%08lx) and CPU (0x%08lx) "
                        "disagree at the 32-bit level — this is a compiler bug\n",
                (unsigned long)(uint32_t)eval_result.value,
                (unsigned long)(uint32_t)cpu_result);
        return EXIT_FAILURE;
    }

    /* ── 7. Result + Level-4 demos ────────────────────────────────────────── */
    printf("\nRESULT: %ld\n", cpu_result);

    run_branch_demo();
    run_loop_demo();
    run_memory_demo();

    return EXIT_SUCCESS;
}

