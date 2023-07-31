#include <stdint.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>

void ldrmain() {
    //ReadSectors(0, 1, (uint32_t*)0x19000);
    SetVideoMode(1024, 768, 32);

    uint32_t bpp = (gVModeInformation.bits_per_pixel + 1) / 8;
    for (int y = 20; y < 40; y++) {
        for (int x = 20; x < 40; x++) {
            *((uint32_t*)(gVModeInformation.physical_base_pointer + (x * bpp) + (y * gVModeInformation.x_resolution * bpp))) = 0xFFDA9623;
        }
    }
    for (;;);
}