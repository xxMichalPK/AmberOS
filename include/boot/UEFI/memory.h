#ifndef __MEMORY_H__
#define __MEMORY_H__

static inline void* memcpy(void* dest, const void* src, UINTN n) {
    UINTN remaining = n;

    asm volatile(
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(remaining)
        :
        : "memory"
    );

    return dest;
}

static inline int memcmp(const void* aptr, const void* bptr, UINTN n){
	const UINT8* a = aptr, *b = bptr;
	for (UINTN i = 0; i < n; i++){
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

#endif