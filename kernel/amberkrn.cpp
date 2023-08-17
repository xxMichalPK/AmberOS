#include <amber.hpp>
#include <boot/bootinfo.h>

__CDECL AMBER_STATUS AmberStartup(BootInfo_t *bootInfo) {
    int test = bootInfo->bootType;
    uint32_t color;
    if (test == UEFI) color = 0xFFFF0000;
    else if (test == BIOS) color = 0xFF00FF00;

    for (int i = 0; i < bootInfo->framebuffer.vRes * bootInfo->framebuffer.hRes; i++) {
        *(uint32_t*)(uintptr_t)(bootInfo->framebuffer.base + i * ((bootInfo->framebuffer.bitsPerPixel + 1) / 8)) = color;
    }

    for (;;);

    // The system shouldn't return. If it does, something went wrong
    return AMBER_FATAL_ERROR;
}