#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Memory subsystem — Level-5 flat RAM.
 *
 * Design:
 *   - 64 KB flat byte-addressable address space.
 *   - All programmer-visible access is 32-bit word-width (4 bytes).
 *   - Addresses must be 4-byte aligned; violations are fatal errors.
 *   - The CPU holds a pointer to Memory but does NOT own it; the caller
 *     (main.c or a test harness) allocates and frees it.
 *
 * Forward-compatibility notes (for future levels):
 *   - TODO(Level-6): insert a cache layer between CPU and mem_read/mem_write.
 *     The cache would intercept these calls, check its tag array, and fall
 *     through to mem_read/mem_write only on a miss.
 *   - TODO(Level-6): add `uint64_t read_count, write_count` statistics fields
 *     to Memory so cache hit-rate analysis has ground truth.
 *   - TODO(Level-7): add a `uint32_t latency_cycles` field per access for
 *     pipeline stall simulation.
 *   - The `data` array is kept as a plain flat buffer intentionally so that
 *     a future virtual-memory layer can overlay page tables on top.
 */

#define MEM_SIZE (64u * 1024u)   /* 64 KB address space */
#define MEM_WORD_SIZE 4u         /* 32-bit word = 4 bytes */

typedef struct {
    uint8_t data[MEM_SIZE];
    /* TODO(Level-6): uint64_t read_count, write_count; */
} Memory;

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

/* Zero-initialize all RAM.  Must be called before any access. */
void mem_init(Memory *mem);

/* ── Word access ──────────────────────────────────────────────────────────── */

/*
 * mem_read_word — load a 32-bit word from address `addr`.
 *
 * Requirements:
 *   addr must be 4-byte aligned.
 *   addr + 4 must be <= MEM_SIZE.
 *
 * On success, stores the value in *out and returns 0.
 * On error (bounds / alignment), prints to stderr and returns -1.
 */
int mem_read_word(const Memory *mem, uint32_t addr, uint32_t *out);

/*
 * mem_write_word — store a 32-bit word at address `addr`.
 *
 * Same alignment and bounds requirements as mem_read_word.
 * On success returns 0; on error prints to stderr and returns -1.
 */
int mem_write_word(Memory *mem, uint32_t addr, uint32_t value);

#endif /* MEMORY_H */
