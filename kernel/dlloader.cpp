#include <dlloader.hpp>
#include <memory/memory.hpp>

int dll_strcmp(const char* str1, const char* str2) {
    while (*str1 != '\0' && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1) - (*str2);
}

// IMPORTANT:
//  We can get the size needed for the GOP by getting its address and size from the .gop* section header
//  that's for the future

// Prepare the dynamic library for use by fixing the addresses, GOT and relocations
void* dlprepare(void* data) {
    Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)data;

    // First move all the data from program headers to their desired addresses
    for (int i = 0; i < elfHeader->e_phnum; i++) {
        Elf64_Phdr* programHeader = (Elf64_Phdr*)(((uintptr_t)data) + elfHeader->e_phoff + (i * elfHeader->e_phentsize));
        if (programHeader->p_vaddr != programHeader->p_offset) {
            memcpy((void*)(((uintptr_t)data) + programHeader->p_vaddr), (void*)(((uintptr_t)data) + programHeader->p_offset), programHeader->p_memsz);
        }
    }

    // Next, process all the relocations
    // Get the Section Header String Table
    Elf64_Shdr* sh_strtab = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + (elfHeader->e_shstrndx * elfHeader->e_shentsize));

    // We're looking for .rela.dyn or .rela.plt section (or both)
    for (int i = 0; i < elfHeader->e_shnum; i++) {
        Elf64_Shdr* sectionHeader = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + (i * elfHeader->e_shentsize));
        const char* sectionName = (const char*)(((uintptr_t)data) + sh_strtab->sh_offset + sectionHeader->sh_name);

        if (dll_strcmp(".rela.dyn\0", sectionName) == 0 || dll_strcmp(".rela.plt\0", sectionName) == 0) {
            for (int r = 0; r < sectionHeader->sh_size / sizeof(Elf64_Rela); r++) {
                Elf64_Rela* rela = (Elf64_Rela*)(((uintptr_t)data) + sectionHeader->sh_offset + (r * sizeof(Elf64_Rela)));
                switch (ELF64_R_TYPE(rela->r_info)) {
                    case R_X86_64_JUMP_SLOT:
                    case R_X86_64_GLOB_DAT: {
                        Elf64_Shdr *symtabSectionHeader = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + sectionHeader->sh_link * elfHeader->e_shentsize);
                        Elf64_Sym* symtab = (Elf64_Sym*)(((uintptr_t)data) + symtabSectionHeader->sh_offset);
                        *((Elf64_Addr*)(((uintptr_t)data) + rela->r_offset)) = (uintptr_t)data + symtab[ELF64_R_SYM(rela->r_info)].st_value;
                        break;
                    }

                    case R_X86_64_64:
                    case R_X86_64_32:
                    case R_X86_64_32S:
                    case R_X86_64_16:
                    case R_X86_64_8: {
                        Elf64_Shdr *symtabSectionHeader = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + sectionHeader->sh_link * elfHeader->e_shentsize);
                        Elf64_Sym* symtab = (Elf64_Sym*)(((uintptr_t)data) + symtabSectionHeader->sh_offset);
                        *((Elf64_Addr*)(((uintptr_t)data) + rela->r_offset)) = (uintptr_t)data + symtab[ELF64_R_SYM(rela->r_info)].st_value + rela->r_addend;
                        break;
                    }

                    default:
                        break;
                }
            }
        }
    }
    
    return data;
}

// Function to find a symbol by name in the symbol table
void* dlsym(void* data, const char* name) {
    Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)data;

    // Find the desired function using dynamic symbol table and string table
    for (int i = 0; i < elfHeader->e_shnum; i++) {
        Elf64_Shdr* sectionHeader = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + i * elfHeader->e_shentsize);
        if (sectionHeader->sh_type == SHT_DYNSYM) {
            Elf64_Sym* dynsym = (Elf64_Sym*)(((uintptr_t)data) + sectionHeader->sh_offset);
            ELF64_R_SYM(dynsym[0].st_value);
            Elf64_Shdr* strtabSectionHeader = (Elf64_Shdr*)(((uintptr_t)data) + elfHeader->e_shoff + sectionHeader->sh_link * elfHeader->e_shentsize);
            char* strtab = (char*)(((uintptr_t)data) + strtabSectionHeader->sh_offset);
            // Find the function in the string table
            for (int s = 0; s < sectionHeader->sh_size / sizeof(Elf64_Sym); s++) {
                if (dll_strcmp(name, strtab + dynsym[s].st_name) == 0) {
                    return (void*)(((uintptr_t)data) + ((uintptr_t)dynsym[s].st_value));
                }
            }
            break;
        }
    }

    return NULL;
}