#include <amber.hpp>

__CDECL AMBER_STATUS AmberStartup() {
    for (;;);

    // The system shouldn't return. If it does, something went wrong
    return AMBER_FATAL_ERROR;
}