#include <memory/memory.hpp>
#include <cpu/avx.hpp>

int __memcmpStandard(const void *ptr1, const void *ptr2, size_t len) {
    int ret = 0;
    __asm__ __volatile__ (
                        // Compare byte by byte
                       "repe cmpsb;\n\t\
                        jz 1f;\n\t"
                        
                        // The values are not the same. Check which is greater and return
                       "movb -1(%%rdi), %%al;\n\t\
                        cmpb -1(%%rsi), %%al;\n\t\
                        jg 0f;\n\t\
                        movq $1, %%rax;\n\t\
                        jmp 2f;\n\t"
                        
                        // The first ptr is smaller (return -1)
                       "0:\n\t\
                            movq $-1, %%rax;\n\t\
                            jmp 2f;\n\t"
                        
                        // Values are the same
                       "1:\n\t\
                            xorq %%rax, %%rax;\n\t\
                            jmp 2f;\n\t"

                        // End of function
                       "2:"
                        : "=a"(ret) : "D"(ptr1), "S"(ptr2), "c"(len) : "memory");
    return ret;
}

int memcmp(const void *ptr1, const void *ptr2, size_t len) {
    return __memcmpStandard(ptr1, ptr2, len);
}