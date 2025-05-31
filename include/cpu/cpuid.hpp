#ifndef __CPUID_HPP__
#define __CPUID_HPP__

#include <cstdint>
#include <cstddef>

namespace CPU::CPUID {

    void cpuid(uint32_t func, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
    void cpuid(uint32_t func, uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

}

#endif