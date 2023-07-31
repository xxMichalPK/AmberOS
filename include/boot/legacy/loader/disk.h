#ifndef __DISK_H__
#define __DISK_H__

#include <boot/legacy/loader/BIOS.h>

typedef struct {
    uint8_t packetSize;
    uint8_t zero;
    uint16_t sectorCount;
    uint16_t offset;
    uint16_t segment;
    uint32_t lba_lo;
    uint32_t lba_hi;
} __attribute__((packed)) DISK_ADDRESS_PACKET_t;

// TODO:
//   This function isn't working. Find out why and fix it!
static int ReadSectors(uint64_t lba, uint16_t numSectors, uint32_t *address) {
    rmode_regs_t regs;
    // Initialize all the segments with the segment of current address
    regs.ds = RMODE_SEGMENT((uint32_t)&_loadAddr);
    regs.es = RMODE_SEGMENT((uint32_t)&_loadAddr);
    regs.fs = RMODE_SEGMENT((uint32_t)&_loadAddr);
    regs.gs = RMODE_SEGMENT((uint32_t)&_loadAddr);

    // Initialize the disk address packet
    DISK_ADDRESS_PACKET_t dap;
    dap.packetSize = 0x10; // Always 16B
    dap.zero = 0x00;
    dap.sectorCount = 1;
    dap.offset = 0x9000;
    dap.segment = 0x1000;
    dap.lba_lo = 16;
    dap.lba_hi = 0;

    // TODO:
    //   0x80 is the disk number. Get it from the bootloader!!
    regs.dx = 0x0080;
    regs.ax = 0x4200;
    regs.si = RMODE_OFFSET(&dap);
    bios_call_wrapper(0x13, &regs);
}

#endif