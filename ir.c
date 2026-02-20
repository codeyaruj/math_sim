#include "ir.h"

#include <stdio.h>
#include <stdlib.h>

/* Initial capacity chosen to cover most real expressions without realloc. */
#define IR_INITIAL_CAPACITY 16

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

void ir_program_init(IRProgram *prog)
{
    prog->data     = malloc(IR_INITIAL_CAPACITY * sizeof(IRInstr));
    if (!prog->data) { perror("malloc"); exit(EXIT_FAILURE); }
    prog->count    = 0;
    prog->capacity = IR_INITIAL_CAPACITY;
}

void ir_program_free(IRProgram *prog)
{
    free(prog->data);
    prog->data     = NULL;
    prog->count    = 0;
    prog->capacity = 0;
}

/* ── Append ───────────────────────────────────────────────────────────────── */

void ir_program_append(IRProgram *prog, IRInstr instr)
{
    if (prog->count == prog->capacity) {
        size_t new_cap = prog->capacity * 2;
        IRInstr *grown = realloc(prog->data, new_cap * sizeof(IRInstr));
        if (!grown) { perror("realloc"); exit(EXIT_FAILURE); }
        prog->data     = grown;
        prog->capacity = new_cap;
    }
    prog->data[prog->count++] = instr;
}

/* ── Helpers ──────────────────────────────────────────────────────────────── */

const char *ir_opcode_name(IROpcode op)
{
    switch (op) {
        case IR_LOAD_CONST: return "LOAD_CONST";
        case IR_ADD:        return "ADD";
        case IR_SUB:        return "SUB";
        case IR_MUL:        return "MUL";
        case IR_DIV:        return "DIV";
        case IR_CMP:        return "CMP";
        case IR_JMP:        return "JMP";
        case IR_JZ:         return "JZ";
        case IR_JNZ:        return "JNZ";
        case IR_LOAD:       return "LOAD";
        case IR_STORE:      return "STORE";
    }
    return "???";
}

void ir_program_dump(const IRProgram *prog)
{
    for (size_t i = 0; i < prog->count; i++) {
        const IRInstr *in = &prog->data[i];
        switch (in->op) {
            case IR_LOAD_CONST:
                fprintf(stderr, "  %2zu  %-12s R%d, %ld\n",
                        i, ir_opcode_name(in->op), in->dst, in->imm);
                break;
            case IR_JMP:
            case IR_JZ:
            case IR_JNZ:
                fprintf(stderr, "  %2zu  %-12s %d\n",
                        i, ir_opcode_name(in->op), in->target);
                break;
            case IR_LOAD:
                fprintf(stderr, "  %2zu  %-12s R%d, [R%d]\n",
                        i, ir_opcode_name(in->op), in->dst, in->addr);
                break;
            case IR_STORE:
                fprintf(stderr, "  %2zu  %-12s R%d, [R%d]\n",
                        i, ir_opcode_name(in->op), in->src, in->addr);
                break;
            default:
                fprintf(stderr, "  %2zu  %-12s R%d, R%d\n",
                        i, ir_opcode_name(in->op), in->dst, in->src);
                break;
        }
    }
}
