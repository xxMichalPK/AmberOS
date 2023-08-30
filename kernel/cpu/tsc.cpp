#include <cpu/cpu.hpp>

namespace CPU {

    inline uint64_t rdtsc() {
        uint64_t tsHigh, tsLow;
        __asm__ __volatile__ ("rdtsc" : "=a"(tsLow), "=d"(tsHigh));
        return ((tsHigh << 32) | tsLow);
    }

};