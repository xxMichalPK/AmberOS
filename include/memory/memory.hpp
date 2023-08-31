#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include <cstdint>
#include <cstddef>

void *memcpy(void *dst, void *src, size_t len);
void *memset(void *dst, int c, size_t len);

#endif