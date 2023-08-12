#ifndef __AMBERERR_HPP__
#define __AMBERERR_HPP__

#include <stdint.h>
#include <cstddef>

typedef size_t AMBER_STATUS;

#ifndef MAX_BIT
    #define MAX_BIT 0x8000000000000000ULL
#endif

#ifndef ENCODE_ERROR
    #define ENCODE_ERROR(_code) ((AMBER_STATUS)(MAX_BIT | (_code)))
#endif

#ifndef AMBER_ERROR
    #define AMBER_ERROR(_code) (((int32_t)(AMBER_STATUS)(_code)) < 0)
#endif


#define AMBER_SUCCESS ((AMBER_STATUS)0)

// Basic errors
#define AMBER_UNKNOWN_ERROR         ENCODE_ERROR(1)
#define AMBER_INVALID_ARGUMENT      ENCODE_ERROR(2)
#define AMBER_LOAD_ERROR            ENCODE_ERROR(3)

// Resource errors
#define AMBER_FILE_NOT_FOUND        ENCODE_ERROR(20)
#define AMBER_CORRUPTED_DATA        ENCODE_ERROR(21)
#define AMBER_OUT_OF_MEMORY         ENCODE_ERROR(22)

// Machine errors
#define AMBER_UNSUPPORTED_FEATURE   ENCODE_ERROR(40)
#define AMBER_CONFIGURATION_ERROR   ENCODE_ERROR(41)
#define AMBER_INVALID_STATE         ENCODE_ERROR(42)
#define AMBER_HARDWARE_ERROR        ENCODE_ERROR(43)

// Security errors
#define AMBER_PERMISSION_DENIED     ENCODE_ERROR(60)
#define AMBER_SECURITY_ERROR        ENCODE_ERROR(61)

// Serious errors
#define AMBER_ABORTED               ENCODE_ERROR(254)
#define AMBER_FATAL_ERROR           ENCODE_ERROR(255)

#endif