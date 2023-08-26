#include <amber.hpp>
#include <boot/bootinfo.h>
#include <cpu/sse.hpp>
#include <cpu/avx.hpp>
#include <memory/memory.hpp>

__CDECL AMBER_STATUS AmberStartup(BootInfo_t *bootInfo) {
    AMBER_STATUS status;

    // Enable SSE instruction set
    status = CPU::SSE::Enable();
    if (AMBER_ERROR(status)) return status;

    // Enable AVX if supported
    CPU::AVX::Enable();

    uint64_t sSize = bootInfo->framebuffer.vRes * bootInfo->framebuffer.hRes;
    uint8_t bpp = ((bootInfo->framebuffer.bitsPerPixel + 1) / 8);

    uint32_t color;
    while (true) {
        for (uint64_t i = 0; i < sSize; i++) {
            *(uint32_t*)(uintptr_t)(0x500000 + i * bpp) = color;
        }

        memcpy((void*)bootInfo->framebuffer.base, (void*)0x500000, sSize * bpp);
        color += 0xFF;
    }

    for (;;);

    // The system shouldn't return. If it does, something went wrong
    return AMBER_FATAL_ERROR;
}