#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#include <stddef.h>

#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/gdt.h>

#define SMALL_PAGE_SIZE 0x1000      // 4KiB
#define MEDIUM_PAGE_SIZE 0x200000   // 2MiB
#define BIG_PAGE_SIZE 0x400000000   // 1GiB

#define PAGE_ADDR(x)        (x & 0x7FFFFFFFFF000)   // The first 12bits of address are 0ed to fit the flags
//#define PT_ENTRY_ADDR(x)    (x & 0x7FFFFFFFFF000)   // The first 12bits of address are 0ed to fit the flags
//#define PD_ENTRY_ADDR(x)    (x & 0x7FFFFFFE00000)   // The first 21bits of address are 0ed to fit the flags
//#define PDPT_ENTRY_ADDR(x)  (x & 0x7FFFFC0000000)   // The first 30bits of address are 0ed to fit the flags

#define PAGE_PRESENT(x)     (x & 1)                 // 1 - Page is present; 0 - not present
#define PAGE_RW(x)          ((x & 1) << 1)          // 1 - Read/Write; 0 - Read only
#define PAGE_USER(x)        ((x & 1) << 2)          // 1 - It's a user mode page; 0 - system page
#define PAGE_WTHROUGH(x)    ((x & 1) << 3)          // 1 - Write Through enabled; 0 - disabled
#define PAGE_CACHE(x)       ((x ^ 1) << 4)          // 1 - Page is cached; 0 - page is not cached
#define PAGE_ACCESSED(x)    ((x & 1) << 5)          // 1 - Page was accessed; 0 - page wasn't accessed
#define PAGE_DIRTY(x)       ((x & 1) << 6)          // 1 - The page is dirty; 0 - not dirty
#define PAGE_BIG(x)         ((x & 1) << 7)          // 1 - The page is a bigger page (2MB or 1GB); 0 - normal size page (4KiB)
                                                    //     This only applies to page tables and directories!
#define PAGE_GLOBAL(x)      ((x & 1) << 8)          // 1 - Page is a global page; 0 - page is not global

#define PAGE_AVL_L(x)       ((x & 7) << 9)          // Lower 3 bits of the available flag
#define PAGE_AVL_H(x)       ((x & 0x7F) << 52)      // Higher 7 bits of the available flag
#define PAGE_PK(x)          ((x & 0xF) << 59)       // Page's protection key
#define PAGE_XD(x)          ((x & 1) << 63)         // Execution Disable bit

extern uint8_t useBigPages;

// For saving space, the loader creates the bigger 2MB pages or 1GB pages if requested and supported!
// It identity maps the first 4GB or 512GB of memory depending on support and requested page size
static int IdentityPaging_Initialize() {
    uint8_t use1Gpages = 0;
    if (useBigPages) {
        uint32_t edx;
        __asm__ __volatile__ ("cpuid" : "=d"(edx) : "a"(0x80000001));
        use1Gpages = (edx << 26) & 0x01;
    }

    uint64_t *pd = NULL;
    if (use1Gpages == 0) {  // If we don't use 1G pages create the page directory and fill it with 2MB pages
        pd = (uint64_t*)lmalloc_a(0x4000, 0x1000);

        for (uint64_t i = 0, p = 0; i < 0x4000 / sizeof(uint64_t); i++) {
            pd[i] = PAGE_ADDR(p) | PAGE_PRESENT(1) | PAGE_RW(1) | PAGE_BIG(1);
            p += MEDIUM_PAGE_SIZE;
        }
    }

    uint64_t *pdpt = (uint64_t*)lmalloc_a(0x1000, 0x1000);
    // If we shouldn't use 1G pages
    if (use1Gpages == 0) {  // Fill the Page Directory Pointer Table with the appropriate page directories
        pdpt[0] = PAGE_ADDR((uint64_t)(uintptr_t)pd) | PAGE_PRESENT(1) | PAGE_RW(1);
        pdpt[1] = PAGE_ADDR(((uint64_t)(uintptr_t)pd) + 0x1000) | PAGE_PRESENT(1) | PAGE_RW(1);
        pdpt[2] = PAGE_ADDR(((uint64_t)(uintptr_t)pd) + 0x2000) | PAGE_PRESENT(1) | PAGE_RW(1);
        pdpt[3] = PAGE_ADDR(((uint64_t)(uintptr_t)pd) + 0x3000) | PAGE_PRESENT(1) | PAGE_RW(1);
    }

    // If we should use the 1G pages
    if (use1Gpages == 1) {  // fill the entire Page Directory Pointer Table with them
        for (uint64_t i = 0, p = 0; i < 0x1000 / sizeof(uint64_t); i++) {
            pdpt[i] = PAGE_ADDR(p) | PAGE_PRESENT(1) | PAGE_RW(1) | PAGE_BIG(1);
            p += BIG_PAGE_SIZE;
        }
    }

    uint64_t *pml4 = (uint64_t*)lmalloc_a(0x1000, 0x1000);
    pml4[0] = PAGE_ADDR((uint64_t)(uintptr_t)pdpt) | PAGE_PRESENT(1) | PAGE_RW(1);

    // Enable PAE
    __asm__ __volatile__ ("mov %cr4, %edx;\n\t\
                           or $(1 << 5), %edx;\n\t\
                           mov %edx, %cr4;");

    // Enable long mode
    __asm__ __volatile__ ("mov $0xC0000080, %ecx;\n\t\
                           rdmsr;\n\t\
                           or $(1 << 8), %eax;\n\t\
                           wrmsr;");

    // Load the PML4 table into cr3 register
    __asm__ __volatile__ ("mov %%eax, %%cr3" :: "a"(pml4));

    // Enable paging
    __asm__ __volatile__ ("mov %%ebx, %%cr0" :: "b"((1 << 31) | (1 << 0)));

    return 0;
}

#endif