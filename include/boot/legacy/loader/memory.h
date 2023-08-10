#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <boot/legacy/loader/BIOS.h>

#define E820_MAX_ENTRIES 128

typedef struct {
    uint64_t baseAddress;
    uint64_t length;
    uint32_t type;
    uint32_t ACPI_Attr;
} __attribute__((packed)) E20_MemoryMap_t;

static inline void* memcpy(void* dest, const void* src, uint32_t n) {
    uint32_t remaining = n;

    asm volatile(
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(remaining)
        :
        : "memory"
    );

    return dest;
}

static inline int memcmp(const void* aptr, const void* bptr, uint32_t n){
	const uint8_t* a = aptr, *b = bptr;
	for (uint32_t i = 0; i < n; i++){
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

static uint32_t gE20MemoryMapEntryCount;
static E20_MemoryMap_t gE20MemoryMap[E820_MAX_ENTRIES];
static int GetE820MemoryMap() {
    rmode_regs_t regs, outRegs;
    // Initialize all the segments with the segment of current address
    regs.ds = RMODE_SEGMENT(&_loadAddr);
    regs.es = RMODE_SEGMENT(&_loadAddr);
    regs.fs = RMODE_SEGMENT(&_loadAddr);
    regs.gs = RMODE_SEGMENT(&_loadAddr);

    regs.eax = 0x0000E820;
    regs.edx = 0x534D4150;                  // SMAP
    regs.ecx = 24;
    regs.edi = RMODE_OFFSET(&gE20MemoryMap[0]);
    gE20MemoryMap[0].ACPI_Attr = 1;         // Force valid ACPI 3.x entry
    bios_call_wrapper(0x15, &regs, &outRegs);
    if (outRegs.eax != 0x534D4150 || outRegs.ebx == 0) return -1;

    gE20MemoryMapEntryCount++;
    uint32_t entryIndex = 1;
    while (outRegs.ebx != 0 && entryIndex < E820_MAX_ENTRIES) {
        regs.eax = 0x0000E820;
        regs.ebx = outRegs.ebx;
        regs.edx = 0x534D4150;                  // SMAP
        regs.ecx = 24;
        regs.edi = RMODE_OFFSET(&gE20MemoryMap[entryIndex]);
        bios_call_wrapper(0x15, &regs, &outRegs);

        if (outRegs.ecx == 0) continue;

        entryIndex++;
        gE20MemoryMapEntryCount++;
    }

    return 0;
}


static uint64_t memoryBase = 0;
static uint64_t freeMemorySize = 0;
static uint64_t currentMemoryRegionIdx = 0;
// Simple bulk allocator
static void InitializeMemoryManager() {
    for (uint32_t idx = 0; idx < gE20MemoryMapEntryCount; idx++) {
        if (gE20MemoryMap[idx].baseAddress >= 0x00100000 && gE20MemoryMap[idx].type == 1) {
            currentMemoryRegionIdx = idx;
            memoryBase = gE20MemoryMap[idx].baseAddress;
            freeMemorySize = gE20MemoryMap[idx].length;
            break;
        }
    }
}

static void* lmalloc(uint64_t size) {
    void* ret = 0;
    if (memoryBase && freeMemorySize >= size) {
        ret = (void*)memoryBase;
        memoryBase += size;
        freeMemorySize -= size;
    }
    return ret;
}

#endif