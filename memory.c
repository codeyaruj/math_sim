#include "memory.h"

#include <stdio.h>
#include <string.h>

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

void mem_init(Memory *mem)
{
    memset(mem->data, 0, sizeof(mem->data));
}

/* ── Internal validation ──────────────────────────────────────────────────── */

/*
 * Validate a 32-bit word access at `addr`.
 *
 * Checks:
 *   1. alignment: addr % 4 == 0
 *   2. bounds:    addr + 4 <= MEM_SIZE
 *
 * Returns 0 on success, -1 with an stderr message on failure.
 */
static int check_access(uint32_t addr, const char *op)
{
    if (addr % MEM_WORD_SIZE != 0) {
        fprintf(stderr,
                "memory error: unaligned %s at address 0x%08x "
                "(must be 4-byte aligned)\n", op, (unsigned)addr);
        return -1;
    }
    /*
     * addr is uint32_t so addr + MEM_WORD_SIZE can overflow if addr is near
     * UINT32_MAX.  Check addr <= MEM_SIZE - MEM_WORD_SIZE instead to stay safe.
     */
    if (addr > (uint32_t)(MEM_SIZE - MEM_WORD_SIZE)) {
        fprintf(stderr,
                "memory error: %s out of bounds at address 0x%08x "
                "(memory size = 0x%x)\n", op, (unsigned)addr, MEM_SIZE);
        return -1;
    }
    return 0;
}

/* ── Word access ──────────────────────────────────────────────────────────── */

/*
 * Little-endian layout (matching x86 / ARM LE):
 *   byte[addr+0] = bits  7:0
 *   byte[addr+1] = bits 15:8
 *   byte[addr+2] = bits 23:16
 *   byte[addr+3] = bits 31:24
 *
 * We assemble/disassemble manually to avoid UB from type-punning via a
 * cast through uint32_t* (strict-aliasing rules).
 *
 * TODO(Level-6): if a cache layer is inserted, these functions should call
 * cache_read_word / cache_write_word instead of accessing data[] directly.
 */

int mem_read_word(const Memory *mem, uint32_t addr, uint32_t *out)
{
    if (!mem) {
        fprintf(stderr, "memory error: NULL memory pointer on read\n");
        return -1;
    }
    if (check_access(addr, "read") != 0) return -1;

    *out = (uint32_t)mem->data[addr]
         | ((uint32_t)mem->data[addr + 1] << 8)
         | ((uint32_t)mem->data[addr + 2] << 16)
         | ((uint32_t)mem->data[addr + 3] << 24);
    return 0;
}

int mem_write_word(Memory *mem, uint32_t addr, uint32_t value)
{
    if (!mem) {
        fprintf(stderr, "memory error: NULL memory pointer on write\n");
        return -1;
    }
    if (check_access(addr, "write") != 0) return -1;

    mem->data[addr]     = (uint8_t)(value        & 0xFFu);
    mem->data[addr + 1] = (uint8_t)((value >> 8)  & 0xFFu);
    mem->data[addr + 2] = (uint8_t)((value >> 16) & 0xFFu);
    mem->data[addr + 3] = (uint8_t)((value >> 24) & 0xFFu);
    return 0;
}
