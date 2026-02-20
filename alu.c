#include "alu.h"

#include <stdio.h>

/* ── Shared constants ─────────────────────────────────────────────────────── */

#define WORD_BITS   32u
#define MSB_MASK    ((word_t)1u << 31)

/* ── Core ripple-carry adder (internal) ───────────────────────────────────── */

/*
 * ripple_add — the foundational building block.
 *
 * Iterates bit-by-bit from LSB to MSB, propagating carry exactly as a
 * hardware ripple-carry adder would.  Both alu_add and alu_sub are built
 * on top of this; they differ only in how they massage `b` and `carry_in`
 * before calling it.
 *
 * Per-bit logic (textbook):
 *   sum_bit   = a_bit ^ b_bit ^ carry_in
 *   carry_out = (a_bit & b_bit) | (carry_in & (a_bit ^ b_bit))
 *
 * carry_in = 0 for addition, 1 for subtraction (a + ~b + 1).
 *
 * Sets Z, N, C on *f; V must be computed by the caller because it depends
 * on the *original* operand signs (before any b-inversion).
 */
static word_t ripple_add(word_t a, word_t b, uint32_t carry_in, ALUFlags *f)
{
    word_t    result = 0;
    uint32_t  carry  = carry_in & 1u;

    for (uint32_t i = 0; i < WORD_BITS; i++) {
        uint32_t a_bit = (a >> i) & 1u;
        uint32_t b_bit = (b >> i) & 1u;

        uint32_t sum_bit   = a_bit ^ b_bit ^ carry;
        uint32_t carry_out = (a_bit & b_bit) | (carry & (a_bit ^ b_bit));

        result |= (word_t)(sum_bit << i);
        carry   = carry_out;
    }

    /* Final carry out of bit 31 is the C flag. */
    f->C = (uint8_t)(carry & 1u);

    /* Zero: all 32 result bits are 0. */
    f->Z = (result == 0u) ? 1u : 0u;

    /* Negative: MSB of result (two's-complement sign bit). */
    f->N = (uint8_t)((result >> 31) & 1u);

    return result;
}

/* ── Public ALU operations ────────────────────────────────────────────────── */

/*
 * alu_add — bit-accurate 32-bit addition.
 *
 * Overflow rule for addition:
 *   V = (sign(a) == sign(b)) && (sign(result) != sign(a))
 * i.e. adding two same-sign numbers that produce the opposite sign.
 */
word_t alu_add(word_t a, word_t b, ALUFlags *f)
{
    word_t result = ripple_add(a, b, 0u, f);

    uint8_t sign_a   = (uint8_t)((a      >> 31) & 1u);
    uint8_t sign_b   = (uint8_t)((b      >> 31) & 1u);
    uint8_t sign_res = (uint8_t)((result >> 31) & 1u);

    /* Signed overflow: same-sign operands produced opposite-sign result. */
    f->V = (uint8_t)((sign_a == sign_b) && (sign_res != sign_a));

    return result;
}

/*
 * alu_sub — two's-complement subtraction via the adder.
 *
 *   a - b  ≡  a + (~b) + 1
 *
 * Passing carry_in=1 into ripple_add with b inverted is identical to the
 * dedicated subtract path in real hardware (no separate subtractor circuit).
 *
 * The carry out of this combined operation satisfies:
 *   C = 1  →  no borrow (a >= b, unsigned)
 *   C = 0  →  borrow    (a <  b, unsigned)
 * This matches ARM's SBC/SUBS carry convention.
 *
 * Overflow rule for subtraction:
 *   V = (sign(a) != sign(b)) && (sign(result) != sign(a))
 * i.e. subtracting operands of opposite signs produced the wrong sign.
 */
word_t alu_sub(word_t a, word_t b, ALUFlags *f)
{
    /*
     * Invert b; pass carry_in=1 so the adder computes a + (~b + 1).
     * ripple_add fills Z, N, C correctly for this combined operation.
     */
    word_t result = ripple_add(a, ~b, 1u, f);

    uint8_t sign_a   = (uint8_t)((a      >> 31) & 1u);
    uint8_t sign_b   = (uint8_t)((b      >> 31) & 1u);
    uint8_t sign_res = (uint8_t)((result >> 31) & 1u);

    /* Signed overflow: opposite-sign operands produced wrong-sign result. */
    f->V = (uint8_t)((sign_a != sign_b) && (sign_res != sign_a));

    return result;
}

/*
 * alu_mul — multiplication (lower 32 bits).
 *
 * The product of two 32-bit values can be 64 bits; we retain only the lower
 * half, consistent with most RISC ISAs (ARM MUL, MIPS MULT lo-word, etc.).
 * C and V are architecturally UNPREDICTABLE for multiply and are zeroed here.
 */
word_t alu_mul(word_t a, word_t b, ALUFlags *f)
{
    /* Cast to 64-bit before multiplying to avoid UB on 32-bit overflow. */
    word_t result = (word_t)((uint64_t)a * (uint64_t)b);

    f->Z = (result == 0u) ? 1u : 0u;
    f->N = (uint8_t)((result >> 31) & 1u);
    f->C = 0;
    f->V = 0;

    return result;
}

/*
 * alu_div — unsigned integer division.
 *
 * Caller MUST verify b != 0 before calling.
 * C and V are architecturally UNPREDICTABLE for divide and are zeroed here.
 */
word_t alu_div(word_t a, word_t b, ALUFlags *f)
{
    word_t result = a / b;

    f->Z = (result == 0u) ? 1u : 0u;
    f->N = (uint8_t)((result >> 31) & 1u);
    f->C = 0;
    f->V = 0;

    return result;
}

/* ── Utility ──────────────────────────────────────────────────────────────── */

void alu_flags_str(const ALUFlags *f, char *buf, int buflen)
{
    snprintf(buf, (size_t)buflen,
             "Z=%u N=%u C=%u V=%u",
             f->Z, f->N, f->C, f->V);
}
