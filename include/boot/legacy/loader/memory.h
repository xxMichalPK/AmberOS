#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <boot/legacy/loader/BIOS.h>

#define E820_MAX_ENTRIES 128
#define MIN_REGION_SIZE 0x10    // This is only used to split one memory region into 2

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
    regs.ds = RMODE_SEGMENT(&GetE820MemoryMap);
    regs.es = RMODE_SEGMENT(&gE20MemoryMap[0]);
    regs.fs = RMODE_SEGMENT(&GetE820MemoryMap);
    regs.gs = RMODE_SEGMENT(&GetE820MemoryMap);

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

// Allocator stuff
typedef struct MemoryRegion_s {
    uint8_t available;
    uint64_t size;
    struct MemoryRegion_s *prev;
    struct MemoryRegion_s *next;
    void *base;
} __attribute__((packed)) MemoryRegion_t;

static MemoryRegion_t* firstMemoryRegion = NULL;
static uint64_t currentMemoryRegionIdx = 0;

// Simple linked list allocator
static void InitializeMemoryManager() {
    for (uint32_t idx = 0; idx < gE20MemoryMapEntryCount; idx++) {
        if (gE20MemoryMap[idx].type == 1 &&
            ((uint32_t)(&_loaderEnd)) >= gE20MemoryMap[idx].baseAddress && 
            ((uint32_t)(&_loaderEnd)) < gE20MemoryMap[idx].baseAddress + gE20MemoryMap[idx].length &&
            gE20MemoryMap[idx].baseAddress + gE20MemoryMap[idx].length - ((uint32_t)(&_loaderEnd)) > 0x10000 // We need at least 65KiB of free memory!
        ) {
            currentMemoryRegionIdx = idx;
            MemoryRegion_t *currentRegion = ((MemoryRegion_t*)(uintptr_t)(uint32_t)(&_loaderEnd));
            firstMemoryRegion = (MemoryRegion_t *)((uintptr_t)currentRegion);
            currentRegion->available = 1;
            currentRegion->prev = NULL;
            currentRegion->next = NULL;
            currentRegion->size = gE20MemoryMap[idx].baseAddress + gE20MemoryMap[idx].length - ((uint32_t)(&_loaderEnd)) - sizeof(MemoryRegion_t);
            currentRegion->base = (void*)(((uintptr_t)currentRegion) + sizeof(MemoryRegion_t));
            break;
        }
    }
}

static void* lmalloc(uint64_t size) {
    MemoryRegion_t *region = firstMemoryRegion;
    while (region) {
        if (region->available == 1 && region->size >= size + sizeof(MemoryRegion_t)) break;
        region = region->next;
    }
    if (!region) return NULL;

    // If there's enaugh space to create another region of size MIN_REGION_SIZE. Create one after the current region
    if (region->size > size + (sizeof(MemoryRegion_t) * 2) + MIN_REGION_SIZE) {
        MemoryRegion_t *newRegion = (MemoryRegion_t *)(uintptr_t)(((uintptr_t)region->base) + size);
        newRegion->available = 1;
        newRegion->prev = region;
        newRegion->next = region->next;
        newRegion->size = (region->size - size - sizeof(MemoryRegion_t));
        newRegion->base = (void*)(((uintptr_t)newRegion) + sizeof(MemoryRegion_t));

        region->next = newRegion;
        region->size = size + sizeof(MemoryRegion_t);
    }

    region->available = 0;
    return region->base;
}

static void* lmalloc_a(uint64_t size, uint32_t alignment) {
    MemoryRegion_t *region = firstMemoryRegion;
    uint64_t adjustment;

    while (region) {
        if (region->available == 1 && region->size >= size + sizeof(MemoryRegion_t)) {
            adjustment = alignment - (((uintptr_t)region->base) % alignment);
            if (adjustment == alignment) adjustment = 0;
            if (adjustment != 0 && adjustment < sizeof(MemoryRegion_t)) adjustment += alignment;
            
            if (region->size >= size + adjustment) break;
        }
        region = region->next;
    }
    if (!region) return NULL;

    MemoryRegion_t *alignedRegion = NULL;
    // If the region isn't aligned create a new, aligned one
    if (adjustment != 0) {
        alignedRegion = (MemoryRegion_t *)(uintptr_t)(((uintptr_t)region->base) + adjustment - sizeof(MemoryRegion_t));
        alignedRegion->available = 0;
        alignedRegion->prev = region;
        alignedRegion->next = region->next;
        alignedRegion->size = (region->size - (adjustment - sizeof(MemoryRegion_t)));
        alignedRegion->base = (void*)(((uintptr_t)alignedRegion) + sizeof(MemoryRegion_t));

        region->next = alignedRegion;
        region->size = adjustment - sizeof(MemoryRegion_t);
    } else {    // Otherwise the address is already aligned
        alignedRegion = region;
        alignedRegion->available = 0;
    }

    // If there's enough space after the aligned region to create a new region - create one
    if (alignedRegion->size > size + (sizeof(MemoryRegion_t) * 2) + MIN_REGION_SIZE) {
        MemoryRegion_t *newRegion = (MemoryRegion_t *)(uintptr_t)(((uintptr_t)alignedRegion->base) + size);
        newRegion->available = 1;
        newRegion->prev = alignedRegion;
        newRegion->next = alignedRegion->next;
        newRegion->size = (alignedRegion->size - size - sizeof(MemoryRegion_t));
        newRegion->base = (void*)(((uintptr_t)newRegion) + sizeof(MemoryRegion_t));

        alignedRegion->next = newRegion;
        alignedRegion->size = size + sizeof(MemoryRegion_t);
    }

    return alignedRegion->base;
}

static void lfree(void *addr) {
    MemoryRegion_t *region = (MemoryRegion_t*)(uintptr_t)(((uintptr_t)addr) - sizeof(MemoryRegion_t));
    region->available = 1;
    while (region->prev && region->prev->available == 1) {
        region->prev->next = region->next;
        region->prev->size += region->size;
        region = region->prev;
    }

    MemoryRegion_t *nextRegion = region->next;
    while (nextRegion && nextRegion->available == 1) {
        region->size += nextRegion->size;
        region->next = nextRegion->next;
        nextRegion = nextRegion->next;
    }
}

#endif