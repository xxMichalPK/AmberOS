#include <memory/memory.hpp>
#include <cpu/avx.hpp>

// Those functions are written in inline assembly to optimize performance
// Input parameters are stored in this order: RDI, RSI, RDX, RCX, R8, R9, (additional parameters are stored on the stack !IN REVERSE ORDER!)

// DISCLAIMER:
//  Avoid using assembly "loop" instruction. It's slow af

inline void *__memsetStandard(void *dst, int c, size_t len) {
    __asm__ __volatile__ ("0:\n\t\
                               movb %%sil, (%%rdi);\n\t\
                               inc %%rdi;\n\t\
                               dec %%rdx;\n\t\
                               jnz 0b;\n\t\
                          " :: "D"(dst), "S"(c), "d"(len) : "memory");
    
    return dst;
}

inline void *__memsetSSE(void *dst, int c, size_t len) {
    // If the length is smaller than 16bytes (one SSE register) use the standard memset
    if (len < 16) {
        return __memsetStandard(dst, c, len);
    }

    // Otherwise use 128bit SSE registers to set the memory
    __asm__ __volatile__ (
                        // Set RCX to the lenght and RDX to 0x0101010101010101
                       "movq %%rdx, %%rcx;\n\t\
                        movq $0x0101010101010101, %%rdx;\n\t"

                        // Set xmm0 to the repeated byte value by multiplying the 'c' value by 0x0101010101010101
                        // and then 'unpacking' it into the xmm0 SSE register
                       "andq $0xFF, %%rsi;\n\t\
                        movq %%rsi, %%rax;\n\t\
                        mulq %%rdx;\n\t\
                        movq %%rax, %%xmm0;\n\t\
                        punpcklbw %%xmm0, %%xmm0;\n\t"

                        // Check for alignment and align if needed by inverting the address
                        // and ANDing it with 15 to get the number of bytes needed for alignment
                       "movq %%rdi, %%rdx;\n\t\
                        neg %%rdx;\n\t\
                        andq $15, %%rdx;\n\t\
                        orb %%dl, %%dl;\n\t\
                        jz 1f;\n\t"
                        
                        // If the address isn't aligned, set the value byte-by-byte until it's aligned
                       "xchg %%rdx, %%rcx;\n\t\
                        subq %%rcx, %%rdx;\n\t\
                        0:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 0b;\n\t\
                            xchg %%rdx, %%rcx;\n\t"

                        // Now, when everything is aligned, calculate the SSE loop size
                       "1:\n\t\
                            movb %%cl, %%dl;\n\t\
                            shrq $4, %%rcx;\n\t\
                            orq %%rcx, %%rcx;\n\t\
                            jz 3f;\n\t"

                        // Execute the actual 'memset' with SSE register xmm0
                       "2:\n\t\
                            movntdq %%xmm0, (%%rdi);\n\t\
                            addq $16, %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 2b;\n\t"

                        // Check if there's any data to set left
                       "3:\n\t\
                            orb %%dl, %%dl;\n\t\
                            jz 5f;\n\t"
                        
                        // If yes, set the remaining bytes, byte-by-byte
                       "    movb %%dl, %%cl;\n\t\
                        4:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            dec %%rcx;\n\t\
                            jnz 4b;\n\t"

                        // Return from the function
                       "5:\n\t"
                        
                        // Set the clobbered registers
                        :: "D"(dst), "S"(c), "d"(len) : "rax", "rcx", "memory");

    return dst;
}

inline void *__memsetAVX(void *dst, int c, size_t len) {
    // If the length is smaller than 32bytes (one AVX register) try using SSE
    if (len < 32) {
        return __memsetSSE(dst, c, len);
    }

    // Otherwise use 256bit AVX registers to set the memory
    __asm__ __volatile__ (
                        // Set RCX to the lenght and RDX to 0x0101010101010101
                       "movq %%rdx, %%rcx;\n\t\
                        movq $0x0101010101010101, %%rdx;\n\t"

                        // Set ymm0 to the repeated byte value by multiplying the 'c' value by 0x0101010101010101
                        // and then permutating it into the ymm0 AVX register
                       "andq $0xFF, %%rsi;\n\t\
                        movq %%rsi, %%rax;\n\t\
                        mulq %%rdx;\n\t\
                        movq %%rax, %%xmm0;\n\t\
                        punpcklbw %%xmm0, %%xmm0;\n\t\
                        vinsertf128 $1, %%xmm0, %%ymm0, %%ymm0;\n\t"

                        // Check for alignment and align if needed by inverting the address
                        // and ANDing it with 31 to get the number of bytes needed for alignment
                       "movq %%rdi, %%rdx;\n\t\
                        neg %%rdx;\n\t\
                        andq $31, %%rdx;\n\t\
                        orb %%dl, %%dl;\n\t\
                        jz 1f;\n\t"
                        
                        // If the address isn't aligned, set the value byte-by-byte until it's aligned
                       "xchg %%rdx, %%rcx;\n\t\
                        subq %%rcx, %%rdx;\n\t\
                        0:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 0b;\n\t\
                            xchg %%rdx, %%rcx;\n\t"

                        // Now, when everything is aligned, calculate the AVX loop size
                       "1:\n\t\
                            movb %%cl, %%dl;\n\t\
                            shrq $5, %%rcx;\n\t\
                            orq %%rcx, %%rcx;\n\t\
                            jz 3f;\n\t"

                        // Execute the actual 'memset' with SSE register xmm0
                       "2:\n\t\
                            vmovntdq %%ymm0, (%%rdi);\n\t\
                            addq $32, %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 2b;\n\t"

                        // Check if there's any data to set left
                       "3:\n\t\
                            orb %%dl, %%dl;\n\t\
                            jz 5f;\n\t"
                        
                        // If yes, set the remaining bytes, byte-by-byte
                       "    movb %%dl, %%cl;\n\t\
                        4:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            dec %%rcx;\n\t\
                            jnz 4b;\n\t"

                        // Return from the function
                       "5:\n\t"
                        
                        // Set the clobbered registers
                        :: "D"(dst), "S"(c), "d"(len) : "rax", "rcx", "memory");

    return dst;
}

inline void *__memsetAVX512(void *dst, int c, size_t len) {
    // If the length is smaller than 64bytes (one AVX512 register) try the AVX method
    if (len < 64) {
        return __memsetAVX(dst, c, len);
    }

    // Otherwise use 256bit AVX registers to set the memory
    __asm__ __volatile__ (
                        // Set RCX to the lenght and RDX to 0x0101010101010101
                       "movq %%rdx, %%rcx;\n\t\
                        movq $0x0101010101010101, %%rdx;\n\t"

                        // Set ymm0 to the repeated byte value by multiplying the 'c' value by 0x0101010101010101
                        // and then permutating it into the ymm0 AVX register
                       "andq $0xFF, %%rsi;\n\t\
                        movq %%rsi, %%rax;\n\t\
                        mulq %%rdx;\n\t\
                        movq %%rax, %%xmm0;\n\t\
                        punpcklbw %%xmm0, %%xmm0;\n\t\
                        vinsertf128 $1, %%xmm0, %%ymm0, %%ymm0;\n\t\
                        vinsertf64x4 $1, %%ymm0, %%zmm0, %%zmm0;\n\t"

                        // Check for alignment and align if needed by inverting the address
                        // and ANDing it with 63 to get the number of bytes needed for alignment
                       "movq %%rdi, %%rdx;\n\t\
                        neg %%rdx;\n\t\
                        andq $63, %%rdx;\n\t\
                        orb %%dl, %%dl;\n\t\
                        jz 1f;\n\t"
                        
                        // If the address isn't aligned, set the value byte-by-byte until it's aligned
                       "xchg %%rdx, %%rcx;\n\t\
                        subq %%rcx, %%rdx;\n\t\
                        0:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 0b;\n\t\
                            xchg %%rdx, %%rcx;\n\t"

                        // Now, when everything is aligned, calculate the AVX512 loop size
                       "1:\n\t\
                            movb %%cl, %%dl;\n\t\
                            shrq $6, %%rcx;\n\t\
                            orq %%rcx, %%rcx;\n\t\
                            jz 3f;\n\t"

                        // Execute the actual 'memset' with SSE register xmm0
                       "2:\n\t\
                            vmovntdq %%zmm0, (%%rdi);\n\t\
                            addq $64, %%rdi;\n\t\
                            decq %%rcx;\n\t\
                            jnz 2b;\n\t"

                        // Check if there's any data to set left
                       "3:\n\t\
                            orb %%dl, %%dl;\n\t\
                            jz 5f;\n\t"
                        
                        // If yes, set the remaining bytes, byte-by-byte
                       "    movb %%dl, %%cl;\n\t\
                        4:\n\t\
                            movb %%sil, (%%rdi);\n\t\
                            incq %%rdi;\n\t\
                            dec %%rcx;\n\t\
                            jnz 4b;\n\t"

                        // Return from the function
                       "5:\n\t"
                        
                        // Set the clobbered registers
                        :: "D"(dst), "S"(c), "d"(len) : "rax", "rcx", "memory");

    return dst;
}

void *memset(void *dst, int c, size_t len) {
    if (!dst || !len) return nullptr;

    // Set data depending on AVX support
    switch (CPU::AVX::version) {
        case CPU::AVX::NO_AVX: 
            return __memsetSSE(dst, c, len);

        case CPU::AVX::AVX:
        case CPU::AVX::AVX2:
            return __memsetAVX(dst, c, len);
        case CPU::AVX::AVX512:
            return __memsetAVX512(dst, c, len);
    }

    return dst;
}