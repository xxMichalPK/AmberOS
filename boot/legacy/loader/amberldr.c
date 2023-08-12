#include <stdint.h>
#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>
#include <boot/legacy/loader/FS/ISO9660.h>

void loader_main(uint8_t bootDriveNum) {
    GetE820MemoryMap();
    InitializeMemoryManager();
    InitializeISO_FS(bootDriveNum);
    uint32_t isobootSize = 0;
    uint8_t *data;
    ReadISO_FSFile(bootDriveNum, "/boot/isoboot.bin", &isobootSize, (void**)&data);
    SetVideoMode(1024, 768, 32);

    uint32_t bpp = (gVModeInformation.bits_per_pixel + 1) / 8;
    for (int y = 0; y < 768; y++) {
        for (int x = 0; x < 1024; x++) {
            *((uint32_t*)(gVModeInformation.physical_base_pointer + (x * bpp) + (y * gVModeInformation.x_resolution * bpp))) = 0xFF04CE28;
        }
    }

    for (;;);
}