#include <stdint.h>
#include <stddef.h>
#include <elf.h>

#include <ambererr.hpp>

#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>
#include <boot/legacy/loader/FS/ISO9660.h>
#include <boot/legacy/loader/gdt.h>
#include <boot/legacy/loader/paging.h>
#include <boot/bootinfo.h>
#include <boot/legacy/loader/configParser.h>
#include <boot/legacy/loader/intop.h>
#include <boot/legacy/loader/textOutput.h>

uint8_t bootDiskType = 0;
uint8_t useBigPages = 0;    // If the loader should use 1G pages for mapping memory (only if supported)

AMBER_STATUS loader_main(uint16_t diskInfo) {
    AMBER_STATUS status;

    uint8_t bootDriveNum = diskInfo & 0xFF;
    bootDiskType = (diskInfo >> 8) & 0xFF;

    status = GetE820MemoryMap();
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to get E20 Memory Map\0");
        return status;
    }

    status = InitializeMemoryManager();
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to initialize the Memory Manager\0");
        return status;
    }

	status = GDT_Initialize();
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to initialize GDT\0");
        return status;
    }
	
    status = ISO_Initialize(bootDriveNum);
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to initialize the ISO FS\0");
        return status;
    }

    // Load the configuration file
    size_t configFileSize;
    uint8_t *configFile;
    status = ISO_ReadFile(bootDriveNum, "/AmberOS/SysConfig/boot.cfg", &configFileSize, (void**)&configFile);
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to read the configuration file\0");
        return status;
    }
    
    // Parse the configuration and store all the needed settings
    uint8_t *vResString, *hResString, *bppString, *useBigPagesString;
    uint32_t vRes, hRes, bpp;
    parseConfig(configFile, "SCREEN SETTINGS\0", "VERTICAL RESOLUTION\0", &vResString);
    parseConfig(configFile, "SCREEN SETTINGS\0", "HORIZONTAL RESOLUTION\0", &hResString);
    parseConfig(configFile, "SCREEN SETTINGS\0", "BITS PER PIXEL\0", &bppString);
    parseConfig(configFile, "MEMORY SETTINGS\0", "USE 1G PAGES\0", &useBigPagesString);
    vRes = atoui((char*)vResString);
    hRes = atoui((char*)hResString);
    bpp = atoui((char*)bppString);
    if (vRes < 320 || hRes < 200 || bpp < 8) {
        vRes = 640;
        hRes = 480;
        bpp = 16;
    }
    if (memcmp(useBigPagesString, "true", 4) == 0) useBigPages = 1;

    // Load the kernel to 1MB
    size_t kernelSize;
    uint8_t *kernel = (uint8_t *)0x00100000;
    status = ISO_ReadFile(bootDriveNum, "/AmberOS/System/amberkrn.elf", &kernelSize, (void**)&kernel);
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to load the kernel\0");
        return status;
    }

	Elf64_Ehdr kernelHeader;
    memcpy(&kernelHeader, kernel, sizeof(kernelHeader));
    if (memcmp(&kernelHeader.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		kernelHeader.e_ident[EI_CLASS] != ELFCLASS64 ||
		kernelHeader.e_ident[EI_DATA] != ELFDATA2LSB ||
		kernelHeader.e_type != ET_EXEC ||
		kernelHeader.e_machine != EM_X86_64 ||
		kernelHeader.e_version != EV_CURRENT
	) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Unsupported kernel ELF file\0");
        return AMBER_CONFIGURATION_ERROR;
	}

    Elf64_Phdr* phdrs;
	{
		size_t size = kernelHeader.e_phnum * kernelHeader.e_phentsize;
        phdrs = (Elf64_Phdr*)lmalloc(size);
        if (!phdrs) {
            setTextFormat(0x4, 0x0);
            lputs("[ ERR ] Failed to allocate memory for the ELF program headers\0");
            return AMBER_OUT_OF_MEMORY;
        }
		memcpy(phdrs, kernel + kernelHeader.e_phoff, size);
	}

    for (Elf64_Phdr* phdr = phdrs; (char*)phdr < (char*)phdrs + kernelHeader.e_phnum * kernelHeader.e_phentsize; phdr = (Elf64_Phdr*)((char*)phdr + kernelHeader.e_phentsize)) {
		switch (phdr->p_type) {
			case PT_LOAD: {
				uintptr_t segment = phdr->p_paddr;
				size_t size = phdr->p_filesz;

                memcpy((void*)segment, kernel + phdr->p_offset, size);
				break;
			}
		}
	}

    // Allocate memory for the boot information before switching to graphics mode
    BootInfo_t *bootInfo = (BootInfo_t *)lmalloc(sizeof(BootInfo_t));
    if (!bootInfo) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to allocate memory for the boot info structure\0");
        return AMBER_OUT_OF_MEMORY;
    }

    status = SetVideoMode(vRes, hRes, (uint8_t)bpp);
    if (AMBER_ERROR(status)) {
        setTextFormat(0x4, 0x0);
        lputs("[ ERR ] Failed to set the Video Mode\0");
        return status;
    }

    bootInfo->bootType = BIOS;
    bootInfo->framebuffer.vRes = gVModeInformation.x_resolution;
    bootInfo->framebuffer.hRes = gVModeInformation.y_resolution;
    bootInfo->framebuffer.size = gVModeInformation.x_resolution * gVModeInformation.y_resolution * ((gVModeInformation.bits_per_pixel + 1) / 8);
    bootInfo->framebuffer.bitsPerPixel = gVModeInformation.bits_per_pixel;
    bootInfo->framebuffer.base = gVModeInformation.physical_base_pointer;

	// Paging should be enabled only after executing all real mode code!
	status = IdentityPaging_Initialize();    // This function also enables 64bit long mode
    if (AMBER_ERROR(status)) return status;

	// Jump to long mode and reload the segment registers to use the 64bit segments
    __asm__ __volatile__ ("pushl %%ebx;\n\t\
                           push $.64reload;\n\t\
                           ljmp *(%%esp);\n\t\
                           \n\t\
                           .64reload:\n\t\
                           add $8, %%esp;\n\t\
                           mov %%ax, %%ds;\n\t\
                           mov %%ax, %%es;\n\t\
                           mov %%ax, %%fs;\n\t\
                           mov %%ax, %%gs;\n\t\
                           mov %%ax, %%ss;\n\t\
						   call *%%ecx;\n\t\
						   .64halt: jmp .64halt;\n\t\
                          " ::
                          "a"(GDT_LMODE_DATA), "b"(GDT_LMODE_CODE), // Pass the Long mode code and data segment offsets
                          "c"((uintptr_t)kernelHeader.e_entry), "D"((uintptr_t)bootInfo)); // 64bit uses System V ABI. We pass the boot info in the edi(rdi) register

    for (;;);

    return AMBER_FATAL_ERROR;
}