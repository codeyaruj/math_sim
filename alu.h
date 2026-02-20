#ifndef ALU_H
#define ALU_H

#include <stdint.h>

/*
 * ALU — Arithmetic Logic Unit
 *
 * All operations are performed on 32-bit unsigned words.  Signed
 * interpretation is derived from bit patterns (two's complement), not from C
 * signed types — this avoids relying on signed overflow UB.
 *
 * word_t is the canonical integer type for the entire Level-3 machine.
 * Every register, immediate (after truncation), and ALU operand uses it.
 */
typedef uint32_t word_t;

/* ── Processor flags ──────────────────────────────────────────────────────── */

/*
 * Standard four-flag set, matching ARM/RISC convention:
 *
 *  Z  — Zero:     result == 0
 *  N  — Negative: MSB of result (bit 31); set when result would be negative
 *                 under two's-complement interpretation
 *  C  — Carry:    unsigned carry out of bit 31 (ADD); complement of borrow
 *                 (SUB, via a+~b+1 method — C=1 means no borrow, a>=b unsigned)
 *  V  — oVerflow: signed overflow; set when the result cannot be represented
 *                 as a signed 32-bit value
 */
typedef struct {
    uint8_t Z;  /* zero     */
    uint8_t N;  /* negative */
    uint8_t C;  /* carry    */
    uint8_t V;  /* overflow */
} ALUFlags;

/* ── ALU operations ───────────────────────────────────────────────────────── */

/*
 * alu_add — ripple-carry addition (bit-accurate)
 *
 *   result = a + b
 *
 * Must NOT use native `+` for the final result; each bit is computed
 * explicitly via carry propagation.
 */
word_t alu_add(word_t a, word_t b, ALUFlags *f);

/*
 * alu_sub — two's-complement subtraction (reuses adder)
 *
 *   result = a - b  ≡  a + (~b) + 1
 *
 * Carry-in of 1 is passed into the ripple adder, which is exactly how
 * hardware subtract units work.
 */
word_t alu_sub(word_t a, word_t b, ALUFlags *f);

/*
 * alu_mul — multiplication (lower 32 bits; may use native *)
 * alu_div — division (may use native /)
 *
 * Caller must ensure b != 0 before calling alu_div.
 * Both ops update Z and N; C and V are architecturally UNPREDICTABLE for
 * these operations (set to 0 here, consistent with ARM behavior).
 */
word_t alu_mul(word_t a, word_t b, ALUFlags *f);
word_t alu_div(word_t a, word_t b, ALUFlags *f);

/* Human-readable flag string written into buf (must be >= 24 bytes). */
void alu_flags_str(const ALUFlags *f, char *buf, int buflen);

#endif /* ALU_H */
