#include <stdint.h>
#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>

void ldrmain(uint8_t bootDriveNum) {
    GetE820MemoryMap();
    InitializeMemoryManager();
    ReadSectors(bootDriveNum, 16, 1, (uint32_t*)0x59000);
    SetVideoMode(1024, 768, 32);

    uint32_t bpp = (gVModeInformation.bits_per_pixel + 1) / 8;
    for (int y = 20; y < 40; y++) {
        for (int x = 20; x < 40; x++) {
            *((uint32_t*)(gVModeInformation.physical_base_pointer + (x * bpp) + (y * gVModeInformation.x_resolution * bpp))) = 0xFFDA9623;
        }
    }

    //uint32_t *test = (uint32_t *)lmalloc(5 * sizeof(uint32_t));
    uint32_t *aligned1 = (uint32_t *)lmalloc_a(0x23, 0x1000);
    uint32_t *aligned2 = (uint32_t *)lmalloc_a(0x23, 0x1000);
    uint32_t *aligned3 = (uint32_t *)lmalloc_a(0x23, 0x1000);
    uint32_t *unaligned = (uint32_t *)lmalloc(0x23);
    for (;;);
}