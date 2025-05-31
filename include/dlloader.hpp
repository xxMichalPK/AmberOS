#ifndef __DL_LOADER_H__
#define __DL_LOADER_H__

#include <cstdint>
#include <cstddef>
#include <elf.h>

void* dlprepare(void* data);
void* dlsym(void* data, const char* name);

#endif