#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include <cstdint>
#include <cstddef>

extern "C" {

void *memcpy(void *dst, void *src, size_t len);
void *memset(void *dst, int c, size_t len);
int memcmp(const void *ptr1, const void *ptr2, size_t len);

}

#endif