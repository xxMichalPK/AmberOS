#include <stdint.h>
#include <stddef.h>
#include <elf.h>

#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/VESA.h>
#include <boot/legacy/loader/disk.h>
#include <boot/legacy/loader/FS/ISO9660.h>
#include <boot/legacy/loader/gdt.h>
#include <boot/legacy/loader/paging.h>

void loader_main(uint8_t bootDriveNum) {
    GetE820MemoryMap();
    InitializeMemoryManager();

	GDT_Initialize();
	
    ISO_Initialize(bootDriveNum);
    size_t kernelSize;
    uint8_t *kernel = (uint8_t *)0x00100000;
    ISO_ReadFile(bootDriveNum, "/AmberOS/System/amberkrn.elf", &kernelSize, (void**)&kernel);

	SetVideoMode(1024, 768, 32);

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

    void (*StartKernel)(uint32_t*) = ((__attribute__((sysv_abi)) void (*)(uint32_t*))(uintptr_t)kernelHeader.e_entry);

	// Paging should be enabled only after executing all real mode code!
	IdentityPaging_Initialize();

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
                          " :: "a"(GDT_LMODE_DATA), "b"(GDT_LMODE_CODE), "c"((uintptr_t)kernelHeader.e_entry), "D"(gVModeInformation.physical_base_pointer));    

    for (;;);
}