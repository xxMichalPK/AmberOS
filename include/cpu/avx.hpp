#ifndef __AVX_HPP__
#define __AVX_HPP__

#include <cstdint>
#include <cstddef>
#include <ambererr.hpp>

namespace CPU::AVX {

    typedef enum {
        NO_AVX = 0x00,
        AVX = 0x01,
        AVX2 = 0x02,
        AVX512 = 0x04
    } AVX_VERSION_t;

    extern AVX_VERSION_t version;
    AMBER_STATUS Enable();

    void Force512();

}

#endif