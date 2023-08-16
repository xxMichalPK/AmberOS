#include <amber.hpp>

__CDECL AMBER_STATUS AmberStartup(uint32_t *fb) {
    for (int i = 0; i < 1024*768; i++) {
        *(uint32_t*)(fb + i) = 0xFFFF3333;
    }

    for (;;);

    // The system shouldn't return. If it does, something went wrong
    return AMBER_FATAL_ERROR;
}