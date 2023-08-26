#include <ambererr.hpp>
#include <cpu/cpuid.hpp>

#define SSE_BIT     25
#define SSE2_BIT    26

namespace CPU::SSE {

    // Enable SSE if supported (minimum version: SSE2)
    AMBER_STATUS Enable() {
        // Check SSE availability
        uint32_t edx, ecx, unused;
        CPUID::cpuid(0x01, &unused, &unused, &ecx, &edx);

        // If SSE2 isn't available return an error
        if ((edx & (1 << SSE_BIT)) == 0 || (edx & (1 << SSE2_BIT)) == 0) {
            return AMBER_UNSUPPORTED_FEATURE;
        }

        // Enable SSE
        __asm__ __volatile__ ("movq %cr0, %rax;\n\t\
                               and $0xFFFB, %ax;\n\t\
                               or $0x02, %ax;\n\t\
                               movq %rax, %cr0;\n\t\
                               movq %cr4, %rax;\n\t\
                               or $(3 << 9), %ax;\n\t\
                               movq %rax, %cr4;");

        return AMBER_SUCCESS;
    }

}