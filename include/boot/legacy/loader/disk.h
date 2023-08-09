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

static int ReadSectors(uint8_t driveNumber, uint64_t lba, uint16_t numSectors, uint32_t *address) {
    // Initialize the disk address packet
    DISK_ADDRESS_PACKET_t dap;
    dap.packetSize = 0x10; // Always 16B
    dap.zero = 0x00;
    dap.sectorCount = numSectors;
    dap.offset = RMODE_OFFSET(address);
    dap.segment = RMODE_SEGMENT(address);
    dap.lba_lo = lba & 0xFFFFFFFF;
    dap.lba_hi = (lba >> 32) & 0xFFFFFFFF;
    
    rmode_regs_t regs;
    // Initialize all the segments with the segment of current address
    regs.ds = RMODE_SEGMENT(&_loadAddr);
    regs.es = RMODE_SEGMENT(&_loadAddr);
    regs.fs = RMODE_SEGMENT(&_loadAddr);
    regs.gs = RMODE_SEGMENT(&_loadAddr);
    regs.dx = driveNumber;
    regs.ax = 0x4200;
    regs.si = RMODE_OFFSET(&dap);
    bios_call_wrapper(0x13, &regs);
}

#endif