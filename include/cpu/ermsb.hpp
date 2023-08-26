#ifndef __ERMSB_HPP__
#define __ERMSB_HPP__

#include <cstdint>
#include <cstddef>
#include <ambererr.hpp>

namespace CPU::ERMSB {

    typedef enum {
        NO_SUPPORT = 0x00,
        ERMSB_SUPPORT = 0x01
    } ERMSB_SUPPORT_t;

    extern ERMSB_SUPPORT_t support;
    AMBER_STATUS Detect();

};

#endif