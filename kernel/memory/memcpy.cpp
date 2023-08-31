#include <memory/memory.hpp>
#include <cpu/avx.hpp>
#include <cpu/ermsb.hpp>

// Those functions are written in inline assembly to optimize performance
// Input parameters are stored in this order: RDI, RSI, RDX, RCX, R8, R9, (additional parameters are stored in stack)

// TODO:
//   Check prefetch line size in cpuid and based on that size execute the prefetches
//   in all the __memcpyXXX functions

// All functions in this file (except the actual memcpy function) are inlined to speed up
// the main memcpy function
inline void *__memcpyStandard(void *dst, void *src, size_t len) {
    __asm__ __volatile__ ("movq %%rdx, %%rcx;\n\t\
                           shrq $3, %%rcx;\n\t\
                           or %%rcx, %%rcx;\n\t\
                           jz 0f;\n\t\
                           rep movsq;\n\t\
                           \n\t\
                           0:\n\t\
                               movb %%dl, %%cl;\n\t\
                               andb $7, %%cl;\n\t\
                               jz 1f;\n\t\
                               rep movsb;\n\t\
                           \n\t\
                           1:" ::: "memory");

    return dst;
}

inline void *__memcpyERMSB(void *dst, void *src, size_t len) {
    __asm__ __volatile__ ("rep movsb" :: "c"(len) : "memory");
    return dst;
}

inline void *__memcpySSE(void *dst, void *src, size_t len) {
    // Before copying check if the size is greater than 256bytes and then if the addresses are aligned to 16bytes (size of SSE registers). 
    // If not, unfortunately we can't use SSE
    if (len < 256 || (((uintptr_t)dst ^ (uintptr_t)src) & 15)) {
        switch (CPU::ERMSB::support) {
            case  CPU::ERMSB::ERMSB_SUPPORT: return __memcpyERMSB(dst, src, len);
            case  CPU::ERMSB::NO_SUPPORT: return __memcpyStandard(dst, src, len);
        }
    }

    size_t remaining;
    void *updatedDst, *updatedSrc;

    __asm__ __volatile__ ("xorq %%rdx, %%rdx;\n\t\
                           movb %%cl, %%dl;\n\t\
                           shrq $8, %%rcx;\n\t\
                           \n\t\
                           0:\n\t\
                              prefetchnta 256(%%rsi);\n\t\
                              prefetchnta 288(%%rsi);\n\t\
                              prefetchnta 320(%%rsi);\n\t\
                              prefetchnta 352(%%rsi);\n\t\
                              prefetchnta 384(%%rsi);\n\t\
                              prefetchnta 416(%%rsi);\n\t\
                              prefetchnta 448(%%rsi);\n\t\
                              prefetchnta 480(%%rsi);\n\t\
                              movdqa 0(%%rsi), %%xmm0;\n\t\
                              movdqa 16(%%rsi), %%xmm1;\n\t\
                              movdqa 32(%%rsi), %%xmm2;\n\t\
                              movdqa 48(%%rsi), %%xmm3;\n\t\
                              movdqa 64(%%rsi), %%xmm4;\n\t\
                              movdqa 80(%%rsi), %%xmm5;\n\t\
                              movdqa 96(%%rsi), %%xmm6;\n\t\
                              movdqa 112(%%rsi), %%xmm7;\n\t\
                              movdqa 128(%%rsi), %%xmm8;\n\t\
                              movdqa 144(%%rsi), %%xmm9;\n\t\
                              movdqa 160(%%rsi), %%xmm10;\n\t\
                              movdqa 176(%%rsi), %%xmm11;\n\t\
                              movdqa 192(%%rsi), %%xmm12;\n\t\
                              movdqa 208(%%rsi), %%xmm13;\n\t\
                              movdqa 224(%%rsi), %%xmm14;\n\t\
                              movdqa 240(%%rsi), %%xmm15;\n\t\
                              movntdq %%xmm0, 0(%%rdi);\n\t\
                              movntdq %%xmm1, 16(%%rdi);\n\t\
                              movntdq %%xmm2, 32(%%rdi);\n\t\
                              movntdq %%xmm3, 48(%%rdi);\n\t\
                              movntdq %%xmm4, 64(%%rdi);\n\t\
                              movntdq %%xmm5, 80(%%rdi);\n\t\
                              movntdq %%xmm6, 96(%%rdi);\n\t\
                              movntdq %%xmm7, 112(%%rdi);\n\t\
                              movntdq %%xmm8, 128(%%rdi);\n\t\
                              movntdq %%xmm9, 144(%%rdi);\n\t\
                              movntdq %%xmm10, 160(%%rdi);\n\t\
                              movntdq %%xmm11, 176(%%rdi);\n\t\
                              movntdq %%xmm12, 192(%%rdi);\n\t\
                              movntdq %%xmm13, 208(%%rdi);\n\t\
                              movntdq %%xmm14, 224(%%rdi);\n\t\
                              movntdq %%xmm15, 240(%%rdi);\n\t\
                              addq $256, %%rsi;\n\t\
                              addq $256, %%rdi;\n\t\
                              decq %%rcx;\n\t\
                              jnz 0b;\n\t\
                           " : "=d"(remaining), "=D"(updatedDst), "=S"(updatedSrc)
                             : "c"(len), "D"(dst), "S"(src));

    if (remaining) {
        switch (CPU::ERMSB::support) {
            case  CPU::ERMSB::ERMSB_SUPPORT: __memcpyERMSB(updatedDst, updatedSrc, remaining);
            case  CPU::ERMSB::NO_SUPPORT: __memcpyStandard(updatedDst, updatedSrc, remaining);
        }
    }

    return dst;
}

inline void *__memcpyAVX(void *dst, void *src, size_t len) {
    // Before copying check if the size is greater than 512bytes and then if the addresses are aligned to 32bytes (size of AVX registers). 
    // If not, check 16byte alignment. If they are 16byte aligned, use SSE instructions.
    if (len < 512 || (((uintptr_t)dst ^ (uintptr_t)src) & 31)) {
        if (len < 256 || (((uintptr_t)dst ^ (uintptr_t)src) & 15)) {
            switch (CPU::ERMSB::support) {
                case  CPU::ERMSB::ERMSB_SUPPORT: return __memcpyERMSB(dst, src, len);
                case  CPU::ERMSB::NO_SUPPORT: return __memcpyStandard(dst, src, len);
            }
        }

        return __memcpySSE(dst, src, len);
    }

    size_t remaining;
    void *updatedDst, *updatedSrc;
    
    __asm__ __volatile__ ("xorq %%rdx, %%rdx;\n\t\
                           movb %%cl, %%dl;\n\t\
                           shrq $9, %%rcx;\n\t\
                           \n\t\
                           0:\n\t\
                              prefetchnta 512(%%rsi);\n\t\
                              prefetchnta 544(%%rsi);\n\t\
                              prefetchnta 576(%%rsi);\n\t\
                              prefetchnta 608(%%rsi);\n\t\
                              prefetchnta 640(%%rsi);\n\t\
                              prefetchnta 672(%%rsi);\n\t\
                              prefetchnta 704(%%rsi);\n\t\
                              prefetchnta 736(%%rsi);\n\t\
                              prefetchnta 768(%%rsi);\n\t\
                              prefetchnta 800(%%rsi);\n\t\
                              prefetchnta 832(%%rsi);\n\t\
                              prefetchnta 864(%%rsi);\n\t\
                              prefetchnta 896(%%rsi);\n\t\
                              prefetchnta 928(%%rsi);\n\t\
                              prefetchnta 960(%%rsi);\n\t\
                              prefetchnta 992(%%rsi);\n\t\
                              vmovdqa 0(%%rsi), %%ymm0;\n\t\
                              vmovdqa 32(%%rsi), %%ymm1;\n\t\
                              vmovdqa 64(%%rsi), %%ymm2;\n\t\
                              vmovdqa 96(%%rsi), %%ymm3;\n\t\
                              vmovdqa 128(%%rsi), %%ymm4;\n\t\
                              vmovdqa 160(%%rsi), %%ymm5;\n\t\
                              vmovdqa 192(%%rsi), %%ymm6;\n\t\
                              vmovdqa 224(%%rsi), %%ymm7;\n\t\
                              vmovdqa 256(%%rsi), %%ymm8;\n\t\
                              vmovdqa 288(%%rsi), %%ymm9;\n\t\
                              vmovdqa 320(%%rsi), %%ymm10;\n\t\
                              vmovdqa 352(%%rsi), %%ymm11;\n\t\
                              vmovdqa 384(%%rsi), %%ymm12;\n\t\
                              vmovdqa 416(%%rsi), %%ymm13;\n\t\
                              vmovdqa 448(%%rsi), %%ymm14;\n\t\
                              vmovdqa 480(%%rsi), %%ymm15;\n\t\
                              vmovntdq %%ymm0, 0(%%rdi);\n\t\
                              vmovntdq %%ymm1, 32(%%rdi);\n\t\
                              vmovntdq %%ymm2, 64(%%rdi);\n\t\
                              vmovntdq %%ymm3, 96(%%rdi);\n\t\
                              vmovntdq %%ymm4, 128(%%rdi);\n\t\
                              vmovntdq %%ymm5, 160(%%rdi);\n\t\
                              vmovntdq %%ymm6, 192(%%rdi);\n\t\
                              vmovntdq %%ymm7, 224(%%rdi);\n\t\
                              vmovntdq %%ymm8, 256(%%rdi);\n\t\
                              vmovntdq %%ymm9, 288(%%rdi);\n\t\
                              vmovntdq %%ymm10, 320(%%rdi);\n\t\
                              vmovntdq %%ymm11, 352(%%rdi);\n\t\
                              vmovntdq %%ymm12, 384(%%rdi);\n\t\
                              vmovntdq %%ymm13, 416(%%rdi);\n\t\
                              vmovntdq %%ymm14, 448(%%rdi);\n\t\
                              vmovntdq %%ymm15, 480(%%rdi);\n\t\
                              addq $512, %%rsi;\n\t\
                              addq $512, %%rdi;\n\t\
                              decq %%rcx;\n\t\
                              jnz 0b;\n\t\
                           " : "=d"(remaining), "=S"(updatedSrc), "=D"(updatedDst)
                             : "c"(len), "S"(src), "D"(dst));

    if (remaining) {
        switch (CPU::ERMSB::support) {
            case  CPU::ERMSB::ERMSB_SUPPORT: __memcpyERMSB(updatedDst, updatedSrc, remaining);
            case  CPU::ERMSB::NO_SUPPORT: __memcpyStandard(updatedDst, updatedSrc, remaining);
        }
    }

    return dst;
}

inline void *__memcpyAVX512(void *dst, void *src, size_t len) {
    // Before copying check if the size is greater than 1024bytes and then if the addresses are aligned to 64bytes (size of AVX512 registers). 
    // If not, check 32byte alignment. If they are 32byte aligned, use AVX instructions, if not, check SSE requirements and proceed copying.
    if (len < 1024 || (((uintptr_t)dst ^ (uintptr_t)src) & 63)) {
        if (len < 512 || (((uintptr_t)dst ^ (uintptr_t)src) & 31)) {
            if (len < 256 || (((uintptr_t)dst ^ (uintptr_t)src) & 15)) {
                switch (CPU::ERMSB::support) {
                    case  CPU::ERMSB::ERMSB_SUPPORT: return __memcpyERMSB(dst, src, len);
                    case  CPU::ERMSB::NO_SUPPORT: return __memcpyStandard(dst, src, len);
                }
            }

            return __memcpySSE(dst, src, len);
        }

        return __memcpyAVX(dst, src, len);
    }

    size_t remaining;
    void *updatedDst, *updatedSrc;
    
    __asm__ __volatile__ ("xorq %%rdx, %%rdx;\n\t\
                           movb %%cl, %%dl;\n\t\
                           shrq $10, %%rcx;\n\t\
                           \n\t\
                           0:\n\t\
                              prefetchnta 1024(%%rsi);\n\t\
                              prefetchnta 1088(%%rsi);\n\t\
                              prefetchnta 1152(%%rsi);\n\t\
                              prefetchnta 1216(%%rsi);\n\t\
                              prefetchnta 1280(%%rsi);\n\t\
                              prefetchnta 1344(%%rsi);\n\t\
                              prefetchnta 1408(%%rsi);\n\t\
                              prefetchnta 1472(%%rsi);\n\t\
                              prefetchnta 1536(%%rsi);\n\t\
                              prefetchnta 1600(%%rsi);\n\t\
                              prefetchnta 1664(%%rsi);\n\t\
                              prefetchnta 1728(%%rsi);\n\t\
                              prefetchnta 1792(%%rsi);\n\t\
                              prefetchnta 1856(%%rsi);\n\t\
                              prefetchnta 1920(%%rsi);\n\t\
                              prefetchnta 1984(%%rsi);\n\t\
                              vmovdqa64 0(%%rsi), %%zmm0;\n\t\
                              vmovdqa64 64(%%rsi), %%zmm1;\n\t\
                              vmovdqa64 128(%%rsi), %%zmm2;\n\t\
                              vmovdqa64 192(%%rsi), %%zmm3;\n\t\
                              vmovdqa64 256(%%rsi), %%zmm4;\n\t\
                              vmovdqa64 320(%%rsi), %%zmm5;\n\t\
                              vmovdqa64 384(%%rsi), %%zmm6;\n\t\
                              vmovdqa64 448(%%rsi), %%zmm7;\n\t\
                              vmovdqa64 512(%%rsi), %%zmm8;\n\t\
                              vmovdqa64 576(%%rsi), %%zmm9;\n\t\
                              vmovdqa64 640(%%rsi), %%zmm10;\n\t\
                              vmovdqa64 704(%%rsi), %%zmm11;\n\t\
                              vmovdqa64 768(%%rsi), %%zmm12;\n\t\
                              vmovdqa64 832(%%rsi), %%zmm13;\n\t\
                              vmovdqa64 896(%%rsi), %%zmm14;\n\t\
                              vmovdqa64 960(%%rsi), %%zmm15;\n\t\
                              vmovntdq %%zmm0, 0(%%rdi);\n\t\
                              vmovntdq %%zmm1, 64(%%rdi);\n\t\
                              vmovntdq %%zmm2, 128(%%rdi);\n\t\
                              vmovntdq %%zmm3, 192(%%rdi);\n\t\
                              vmovntdq %%zmm4, 256(%%rdi);\n\t\
                              vmovntdq %%zmm5, 320(%%rdi);\n\t\
                              vmovntdq %%zmm6, 384(%%rdi);\n\t\
                              vmovntdq %%zmm7, 448(%%rdi);\n\t\
                              vmovntdq %%zmm8, 512(%%rdi);\n\t\
                              vmovntdq %%zmm9, 576(%%rdi);\n\t\
                              vmovntdq %%zmm10, 640(%%rdi);\n\t\
                              vmovntdq %%zmm11, 704(%%rdi);\n\t\
                              vmovntdq %%zmm12, 768(%%rdi);\n\t\
                              vmovntdq %%zmm13, 832(%%rdi);\n\t\
                              vmovntdq %%zmm14, 896(%%rdi);\n\t\
                              vmovntdq %%zmm15, 960(%%rdi);\n\t\
                              addq $1024, %%rsi;\n\t\
                              addq $1024, %%rdi;\n\t\
                              decq %%rcx;\n\t\
                              jnz 0b;\n\t\
                           " : "=d"(remaining), "=S"(updatedSrc), "=D"(updatedDst)
                             : "c"(len), "S"(src), "D"(dst));

    if (remaining) {
        switch (CPU::ERMSB::support) {
            case  CPU::ERMSB::ERMSB_SUPPORT: __memcpyERMSB(updatedDst, updatedSrc, remaining);
            case  CPU::ERMSB::NO_SUPPORT: __memcpyStandard(updatedDst, updatedSrc, remaining);
        }
    }

    return dst;
}

void *memcpy(void *dst, void *src, size_t len) {
    if (!dst || !src || !len) return nullptr;

    // Copy depending on AVX support
    switch (CPU::AVX::version) {
        case CPU::AVX::NO_AVX: 
            return __memcpySSE(dst, src, len);

        case CPU::AVX::AVX:
        case CPU::AVX::AVX2:
            return __memcpyAVX(dst, src, len);
        case CPU::AVX::AVX512:
            return __memcpyAVX512(dst, src, len);
    }

    return dst;
}