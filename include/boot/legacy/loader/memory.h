#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

static inline void* memcpy(void* dest, const void* src, uint32_t n) {
    uint32_t remaining = n;

    asm volatile(
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(remaining)
        :
        : "memory"
    );

    return dest;
}

static inline int memcmp(const void* aptr, const void* bptr, uint32_t n){
	const uint8_t* a = aptr, *b = bptr;
	for (uint32_t i = 0; i < n; i++){
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

#endif