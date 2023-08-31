#include <cpu/avx.hpp>
#include <cpu/cpuid.hpp>

#include <ambererr.hpp>

#define XSAVE_BIT   26
#define AVX_BIT     28

#define AVX2_BIT    5
#define AVX512F_BIT 16

namespace CPU::AVX {

    AVX_VERSION_t version;

    // Enable AVX and AVX512 if supported
    AMBER_STATUS Enable() {
        // Check AVX availability
        uint32_t ebx, ecx, edx, unused;
        CPUID::cpuid(0x01, &unused, &unused, &ecx, &edx);

        // If AVX isn't available return an error
        if ((ecx & (1 << AVX_BIT)) == 0 || (ecx & (1 << XSAVE_BIT)) == 0) {
            version = NO_AVX;
            return AMBER_UNSUPPORTED_FEATURE;
        }

        // For now set the version to standard AVX
        version = AVX;
        // If avx is supported enable the XGETBV and XSETBV instructions by setting bit 18 in CR4
        __asm__ __volatile__ ("movq %cr4, %rax;\n\t\
                               orq $(1 << 18), %rax\n\t\
                               movq %rax, %cr4;");

        // If the bit is set. We can now enable AVX extensions
        __asm__ __volatile__ ("push %rax;\n\t\
                               push %rcx;\n\t\
                               push %rdx;\n\t\
                               xor %rcx, %rcx;\n\t\
                               xgetbv;\n\t\
                               or $7, %eax;\n\t\
                               xsetbv;\n\t\
                               pop %rdx;\n\t\
                               pop %rcx;\n\t\
                               pop %rax;");

        // Check for higher versions of AVX, (AVX2 and AVX512)
        CPUID::cpuid(7, 0, &unused, &ebx, &ecx, &edx);
        if ((ebx & (1 << AVX2_BIT))) version = AVX2;
        if ((ebx & (1 << AVX512F_BIT))) {
            // If AVX512 is supported, enable it by changing xcr0 flags
            // Unfortunately I can't check if this works because bochs returns empty registers 
            // on this specific cpuid call (probably it's not fully supported)
            // But according to specification, it should work
            __asm__ __volatile__ ("push %rax;\n\t\
                                   push %rcx;\n\t\
                                   push %rdx;\n\t\
                                   xor %rcx, %rcx;\n\t\
                                   xgetbv;\n\t\
                                   or $0xE7, %eax;\n\t\
                                   xsetbv;\n\t\
                                   pop %rdx;\n\t\
                                   pop %rcx;\n\t\
                                   pop %rax;");
            version = AVX512;
        }

        return AMBER_SUCCESS;
    }

    // This is a function that will be probably removed from the release versions. It's used to test
    // AVX512 instruction set on Bochs, since it doesn't show support in the CPUID instruction, idk why
    void Force512() {
        __asm__ __volatile__ ("push %rax;\n\t\
                                   push %rcx;\n\t\
                                   push %rdx;\n\t\
                                   xor %rcx, %rcx;\n\t\
                                   xgetbv;\n\t\
                                   or $0xE7, %eax;\n\t\
                                   xsetbv;\n\t\
                                   pop %rdx;\n\t\
                                   pop %rcx;\n\t\
                                   pop %rax;");
            version = AVX512;
    }

}