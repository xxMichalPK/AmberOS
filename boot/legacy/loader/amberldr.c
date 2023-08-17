#include <stdint.h>
#include <stddef.h>
#include <elf.h>

#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>
#include <boot/legacy/loader/FS/ISO9660.h>
#include <boot/legacy/loader/gdt.h>
#include <boot/legacy/loader/paging.h>
#include <boot/bootinfo.h>
#include <boot/legacy/loader/configParser.h>
#include <boot/legacy/loader/intop.h>

void loader_main(uint8_t bootDriveNum) {
    GetE820MemoryMap();
    InitializeMemoryManager();

	GDT_Initialize();
	
    ISO_Initialize(bootDriveNum);

    BootInfo_t *bootInfo = (BootInfo_t *)lmalloc(sizeof(BootInfo_t));
    if (!bootInfo) return;

    // Load the configuration file
    size_t configFileSize;
    uint8_t *configFile = (uint8_t*)lmalloc(0x1000);    // There's a bug when using empty buffer so we allocate 4KiB of memory to fit the configuration for now.
    ISO_ReadFile(bootDriveNum, "/AmberOS/SysConfig/boot.cfg", &configFileSize, (void**)&configFile);
    
    // Parse the configuration and store all the needed settings
    uint8_t *vResString, *hResString, *bppString;
    uint32_t vRes, hRes, bpp;
    parseConfig(configFile, "SCREEN SETTINGS\0", "VERTICAL RESOLUTION\0", &vResString);
    parseConfig(configFile, "SCREEN SETTINGS\0", "HORIZONTAL RESOLUTION\0", &hResString);
    parseConfig(configFile, "SCREEN SETTINGS\0", "BITS PER PIXEL\0", &bppString);
    vRes = atoui((char*)vResString);
    hRes = atoui((char*)hResString);
    bpp = atoui((char*)bppString);

    // Load the kernel to 1MB
    size_t kernelSize;
    uint8_t *kernel = (uint8_t *)0x00100000;
    ISO_ReadFile(bootDriveNum, "/AmberOS/System/amberkrn.elf", &kernelSize, (void**)&kernel);

	SetVideoMode(vRes, hRes, (uint8_t)bpp);

	Elf64_Ehdr kernelHeader;
    memcpy(&kernelHeader, kernel, sizeof(kernelHeader));
    if (memcmp(&kernelHeader.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		kernelHeader.e_ident[EI_CLASS] != ELFCLASS64 ||
		kernelHeader.e_ident[EI_DATA] != ELFDATA2LSB ||
		kernelHeader.e_type != ET_EXEC ||
		kernelHeader.e_machine != EM_X86_64 ||
		kernelHeader.e_version != EV_CURRENT
	) {
        return;
	}

    Elf64_Phdr* phdrs;
	{
		size_t size = kernelHeader.e_phnum * kernelHeader.e_phentsize;
        phdrs = (Elf64_Phdr*)lmalloc(size);
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

    bootInfo->bootType = BIOS;
    bootInfo->framebuffer.vRes = gVModeInformation.x_resolution;
    bootInfo->framebuffer.hRes = gVModeInformation.y_resolution;
    bootInfo->framebuffer.size = gVModeInformation.x_resolution * gVModeInformation.y_resolution * ((gVModeInformation.bits_per_pixel + 1) / 8);
    bootInfo->framebuffer.bitsPerPixel = gVModeInformation.bits_per_pixel;
    bootInfo->framebuffer.base = gVModeInformation.physical_base_pointer;

	// Paging should be enabled only after executing all real mode code!
	IdentityPaging_Initialize();    // This function also enables 64bit long mode

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
}