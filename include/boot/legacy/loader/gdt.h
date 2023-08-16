#ifndef __GDT_H__
#define __GDT_H__

#include <stdio.h>
#include <stdint.h>

#include <boot/legacy/loader/memory.h>

#define GDT_ENTRY_COUNT 7

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Source: https://wiki.osdev.org/GDT_Tutorial
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x01) // Long mode
#define SEG_SIZE(x)      ((x) << 0x02) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x03) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)
 
#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed
///////////////////////////////////////////////////////////////////////////////////////////////////////

#define GDT_CODE_ACCESS_BYTE    SEG_PRES(1) | SEG_PRIV(0) | SEG_DESCTYPE(1) | SEG_CODE_EXRD
#define GDT_DATA_ACCESS_BYTE    SEG_PRES(1) | SEG_PRIV(0) | SEG_DESCTYPE(1) | SEG_DATA_RDWR

#define GDT_RMODE_FLAGS         SEG_GRAN(0) | SEG_SIZE(0) | SEG_LONG(0)
#define GDT_PMODE_FLAGS         SEG_GRAN(1) | SEG_SIZE(1) | SEG_LONG(0)
#define GDT_LMODE_FLAGS         SEG_GRAN(1) | SEG_SIZE(0) | SEG_LONG(1)

typedef struct {
    uint16_t limit_l;
    uint16_t base_l;
    uint8_t base_m;
    uint8_t access_byte;
    uint8_t limit_h : 4;
    uint8_t flags : 4;
    uint8_t base_h;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t size;
    uint64_t addr;
} __attribute__((packed)) gdtr_t;

static void GDT_SetDescriptor(gdt_entry_t *gdt, uint32_t base, uint32_t limit, uint8_t accessByte, uint8_t flags) {
    // Set the base
    gdt->base_l = base & 0x0000FFFF;
    gdt->base_m = base & 0x00FF0000;
    gdt->base_h = base & 0xFF000000;

    // Set the limit
    gdt->limit_l = limit & 0x0000FFFF;
    gdt->limit_h = limit & 0x000F0000;
    
    // Set the access byte
    gdt->access_byte = accessByte;

    // Set the flags
    gdt->flags = flags & 0x0F;
}


static gdtr_t *gdtr = NULL;
static gdt_entry_t *gdt = NULL;

// Those are the offsets to the GDT segments
static uint16_t GDT_RMODE_CODE;
static uint16_t GDT_RMODE_DATA;

static uint16_t GDT_PMODE_CODE;
static uint16_t GDT_PMODE_DATA;

static uint16_t GDT_LMODE_CODE;
static uint16_t GDT_LMODE_DATA;

static int GDT_Initialize() {
    gdt = (gdt_entry_t*)lmalloc(GDT_ENTRY_COUNT * sizeof(gdt_entry_t));
    if (!gdt) return -1;

    for (uint32_t i = 0; i < GDT_ENTRY_COUNT * sizeof(gdt_entry_t); i++) {
        *((uint8_t*)gdt + i) = 0;
    }

    // Set the null descriptor
    GDT_SetDescriptor(&gdt[0], 0, 0, 0, 0);

    // Set 16bit real mode descriptor values
    GDT_SetDescriptor(&gdt[1], 0, 0xFFFFF, GDT_CODE_ACCESS_BYTE, GDT_RMODE_FLAGS);
    GDT_SetDescriptor(&gdt[2], 0, 0xFFFFF, GDT_DATA_ACCESS_BYTE, GDT_RMODE_FLAGS);

    // Set 32bit protected mode descriptor values
    GDT_SetDescriptor(&gdt[3], 0, 0xFFFFF, GDT_CODE_ACCESS_BYTE, GDT_PMODE_FLAGS);
    GDT_SetDescriptor(&gdt[4], 0, 0xFFFFF, GDT_DATA_ACCESS_BYTE, GDT_PMODE_FLAGS);

    // Set 64bit long mode descriptor values
    GDT_SetDescriptor(&gdt[5], 0, 0xFFFFF, GDT_CODE_ACCESS_BYTE, GDT_LMODE_FLAGS);
    GDT_SetDescriptor(&gdt[6], 0, 0xFFFFF, GDT_DATA_ACCESS_BYTE, GDT_LMODE_FLAGS);

    uint64_t gdt_start = (uint64_t)(uintptr_t)&gdt[0];
    // Calculate the offsets
    GDT_RMODE_CODE = ((uint64_t)(uintptr_t)&gdt[1]) - gdt_start;
    GDT_RMODE_DATA = ((uint64_t)(uintptr_t)&gdt[2]) - gdt_start;

    GDT_PMODE_CODE = ((uint64_t)(uintptr_t)&gdt[3]) - gdt_start;
    GDT_PMODE_DATA = ((uint64_t)(uintptr_t)&gdt[4]) - gdt_start;

    GDT_LMODE_CODE = ((uint64_t)(uintptr_t)&gdt[5]) - gdt_start;
    GDT_LMODE_DATA = ((uint64_t)(uintptr_t)&gdt[6]) - gdt_start;

    gdtr = (gdtr_t*)lmalloc(sizeof(gdtr_t));
    if (!gdtr) {
        lfree(gdt);
        gdt = NULL;
        return -1;
    }

    gdtr->size = GDT_ENTRY_COUNT * sizeof(gdt_entry_t) - 1;
    gdtr->addr = (uint64_t)(uintptr_t)gdt;

    // Load the new Global Descriptor Table and far jump to ensure valid CS:IP
    __asm__ __volatile__ ("lgdt (%%edx);\n\t\
                           mov %%ax, %%ds;\n\t\
                           mov %%ax, %%es;\n\t\
                           mov %%ax, %%fs;\n\t\
                           mov %%ax, %%gs;\n\t\
                           mov %%ax, %%ss;\n\t\
                           pushl %%ebx;\n\t\
                           push $.setcs;\n\t\
                           ljmp *(%%esp);\n\t\
                           \n\t\
                           .setcs:\n\t\
                           add $8, %%esp;\n\t\
                          " :: "a"(GDT_PMODE_DATA), "b"(GDT_PMODE_CODE), "d"(gdtr));

    return 0;
}

#endif