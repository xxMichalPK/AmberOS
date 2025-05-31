#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    BIOS = 0x00,
    UEFI = 0x01
} __attribute__ ((packed)) BootType_t;

typedef struct {
    uint32_t vRes;
    uint32_t hRes;
    uint64_t size;
    uint8_t bitsPerPixel;
    uint64_t base;
} __attribute__ ((packed)) Framebuffer_t;

typedef struct {
    BootType_t bootType;
    Framebuffer_t framebuffer;
} __attribute__ ((packed)) BootInfo_t;

#endif