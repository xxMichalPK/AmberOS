#ifndef __BIOS_H__
#define __BIOS_H__

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint32_t edi, esi, ebx, edx, ecx, eax;
    uint16_t gs, fs, es, ds;
} rmode_regs_t;

extern void bios_call_wrapper(uint8_t interruptNumber, rmode_regs_t *regs);
extern uint32_t _loadAddr;

#define RMODE_OFFSET(x) (uint16_t)(((uint32_t)x) & 0xFFFF)
#define RMODE_SEGMENT(x) (uint16_t)((((uint32_t)x) - RMODE_OFFSET(x)) >> 4)

#endif