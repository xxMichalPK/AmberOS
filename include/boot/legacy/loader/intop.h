#ifndef __INTOP_H__
#define __INTOP_H__

#include <stdint.h>
#include <stddef.h>

static int32_t atoi(char* str) {
    int32_t num = 0;
    while (*str != '\0' && *str >= '0' && *str <= '9') {
        num *= 10;
        num += (*str) - '0';
        str++;
    }
    return num;
}

static uint32_t atoui(char* str) {
    uint32_t num = 0;
    while (*str != '\0' && *str >= '0' && *str <= '9') {
        num *= 10;
        num += (*str) - '0';
        str++;
    }
    return num;
}

static int64_t atol(char* str) {
    int64_t num = 0;
    while (*str != '\0' && *str >= '0' && *str <= '9') {
        num *= 10;
        num += (*str) - '0';
        str++;
    }
    return num;
}

static uint64_t atoul(char* str) {
    uint64_t num = 0;
    while (*str != '\0' && *str >= '0' && *str <= '9') {
        num *= 10;
        num += (*str) - '0';
        str++;
    }
    return num;
}

#endif