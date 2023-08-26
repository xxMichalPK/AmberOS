#include <cpu/ermsb.hpp>
#include <cpu/cpuid.hpp>

#define ERMSB_BIT 9

namespace CPU::ERMSB {

    ERMSB_SUPPORT_t support;
    AMBER_STATUS Detect() {
        uint32_t unused, ebx;
        CPUID::cpuid(7, 0, &unused, &ebx, &unused, &unused);

        if ((ebx & (1 << ERMSB_BIT))) {
            support = ERMSB_SUPPORT;
            return AMBER_SUCCESS;
        }

        support = NO_SUPPORT;
        return AMBER_UNSUPPORTED_FEATURE;
    }

};