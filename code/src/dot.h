#ifndef DOT_H
#define DOT_H
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>
#include "dot_utils.h"

/***************************************** Precise Variants *****************************************/


#define __ADD_N_8(result, a, b, c_in, c_out)                                                         \
    do                                                                                               \
    {                                                                                                \
        /* a_vec [511:0] := MEM [mem_addr + 511:mem_addr] */                                         \
        __m512i a_vec = _mm512_loadu_si512((__m512i *)(a));                                          \
        /* b_vec [511:0] := MEM [mem_addr + 511:mem_addr] */                                         \
        __m512i b_vec = _mm512_loadu_si512((__m512i *)(b));                                          \
        /* FOR j := 0 to 7 */                                                                        \
        /*   i := j*64 */                                                                            \
        /*   result_vec [i+63:i] := a_vec [i+63:i] + b_vec [i+63:i] */                               \
        /* ENDFOR */                                                                                 \
        __m512i result_vec = _mm512_add_epi64(a_vec, b_vec);                                         \
        /* FOR j := 0 to 7 */                                                                        \
        /*   i := j*64 */                                                                            \
        /*   c_mask[j] := ( result_vec[i+63:i] < a_vec[i+63:i] ) ? 1 : 0 */                          \
        /* ENDFOR */                                                                                 \
        __mmask16 c_mask = _mm512_cmplt_epu64_mask(result_vec, a_vec);                               \
        c_out = c_mask >> 7;                                                                         \
        c_mask <<= 1;                                                                                \
        /* c_mask[15:0] := c_mask[15:0] OR c_in[15:0] */                                             \
        c_mask = _mm512_kor(c_mask, c_in);                                                           \
        /* FOR j := 0 to 7 */                                                                        \
        /*   i := j*64 */                                                                            \
        /*   IF c_mask[j] */                                                                         \
        /*     result_vec_new [i+63:i] := result_vec [i+63:i] - AVX512_MASK [i+63:i] */              \
        /*   ELSE */                                                                                 \
        /*     result_vec_new [i+63:i] := result_vec [i+63:i] */                                     \
        /*   FI */                                                                                   \
        /* ENDFOR */                                                                                 \
        __m512i result_vec_new = _mm512_mask_sub_epi64(result_vec, c_mask, result_vec, AVX512_MASK); \
        /* FOR j := 0 to 7 */                                                                        \
        /*   i := j*64 */                                                                            \
        /*   c_mask[j] := ( result_vec_new[i+63:i] < result_vec[i+63:i] ) ? 1 : 0 */                 \
        /* ENDFOR */                                                                                 \
        c_mask = _mm512_cmplt_epu64_mask(result_vec_new, result_vec);                                \
        result_vec = result_vec_new;                                                                 \
        /* MEM[mem_addr+511:mem_addr] := result_vec[511:0] */                                        \
        _mm512_storeu_si512((__m512i *)(result), result_vec);                                        \
        /* IF c_mask[15:0] != 0 */                                                                   \
        if (unlikely(_mm512_mask2int(c_mask)))                                                       \
        {                                                                                            \
            c_mask <<= 1;                                                                            \
            /* FOR j := 0 to 7 */                                                                    \
            /*   i := j*64 */                                                                        \
            /* m[j] := ( result_vec[i+63:i] == AVX512_MASK[i+63:i] ) ? 1 : 0*/                       \
            /* ENDFOR */                                                                             \
            __mmask16 m = _mm512_cmpeq_epi64_mask(result_vec, AVX512_MASK);                          \
            c_mask = c_mask + m;                                                                     \
            /* c_out[15:0] := c_out[15:0] OR (c_mask >> 8)[15:0] */                                  \
            c_out = _mm512_kor(c_out, (c_mask >> 8));                                                \
            /* m[15:0] := c_mask[15:0] OR m[15:0] */                                                 \
            m = _mm512_kxor(c_mask, m);                                                              \
            /* FOR j := 0 to 7 */                                                                    \
            /* i := j*64 */                                                                          \
            /* IF k[j] */                                                                            \
            /*	dst[i+63:i] := a[i+63:i] - b[i+63:i] */                                               \
            /* ELSE */                                                                               \
            /*  dst[i+63:i] := src[i+63:i] */                                                        \
            /* FI	 */                                                                                \
            /* ENDFOR   */                                                                           \
            result_vec = _mm512_mask_sub_epi64(result_vec, m, result_vec, AVX512_MASK);              \
            /* MEM[mem_addr+511:mem_addr] := result_vec[511:0] */                                    \
            _mm512_storeu_si512((__m512i *)(result), result_vec);                                    \
        }                                                                                            \
    } while (0)

#define __ADD_N_K(result, a, b, c_in, c_out, k, remaining)                                           \
    do                                                                                               \
    {                                                                                                \
        __m512i a_vec = _mm512_mask_loadu_epi64(AVX512_ZEROS, k, (__m512i *)(a));                    \
        __m512i b_vec = _mm512_mask_loadu_epi64(AVX512_ZEROS, k, (__m512i *)(b));                    \
        __m512i result_vec = _mm512_add_epi64(a_vec, b_vec);                                         \
        __mmask16 c_mask = _mm512_cmplt_epu64_mask(result_vec, a_vec);                               \
        c_out = c_mask >> remaining;                                                                 \
        c_mask <<= 1;                                                                                \
        c_mask = _mm512_kor(c_mask, c_in);                                                           \
        __m512i result_vec_new = _mm512_mask_sub_epi64(result_vec, c_mask, result_vec, AVX512_MASK); \
        c_mask = _mm512_cmplt_epu64_mask(result_vec_new, result_vec);                                \
        result_vec = result_vec_new;                                                                 \
        _mm512_mask_storeu_epi64((__m512i *)(result), k, result_vec);                                \
        if (unlikely(_mm512_mask2int(c_mask)))                                                       \
        {                                                                                            \
            c_mask <<= 1;                                                                            \
            __mmask16 m = _mm512_cmpeq_epi64_mask(result_vec, AVX512_MASK);                          \
            c_mask = c_mask + m;                                                                     \
            c_out = _mm512_kor(c_out, (c_mask >> (remaining + 1)));                                  \
            m = _mm512_kxor(c_mask, m);                                                              \
            result_vec = _mm512_mask_sub_epi64(result_vec, m, result_vec, AVX512_MASK);              \
            _mm512_mask_storeu_epi64((__m512i *)(result), k, result_vec);                            \
        }                                                                                            \
    } while (0)


#define __SUB_N_8(result, a, b, b_in, b_out)                                                         \
    do                                                                                               \
    {                                                                                                \
        __m512i a_vec = _mm512_loadu_si512((__m512i *)(a));                                          \
        __m512i b_vec = _mm512_loadu_si512((__m512i *)(b));                                          \
        __m512i result_vec = _mm512_sub_epi64(a_vec, b_vec);                                         \
        __mmask16 b_mask = _mm512_cmpgt_epu64_mask(b_vec, a_vec);                                    \
        b_out = b_mask >> 7;                                                                         \
        b_mask <<= 1;                                                                                \
        b_mask = _mm512_kor(b_mask, b_in);                                                           \
        __m512i result_vec_new = _mm512_mask_add_epi64(result_vec, b_mask, result_vec, AVX512_MASK); \
        b_mask = _mm512_cmpgt_epu64_mask(result_vec_new, result_vec);                                \
        result_vec = result_vec_new;                                                                 \
        _mm512_storeu_si512((__m512i *)(result), result_vec);                                        \
        if (unlikely(_mm512_mask2int(b_mask)))                                                       \
        {                                                                                            \
            b_mask <<= 1;                                                                            \
            __mmask16 m = _mm512_cmpeq_epu64_mask(result_vec, AVX512_ZEROS);                         \
            b_mask = b_mask + m;                                                                     \
            b_out = _mm512_kor(b_out, (b_mask >> 8));                                                \
            m = _mm512_kxor(b_mask, m);                                                              \
            result_vec = _mm512_mask_add_epi64(result_vec, m, result_vec, AVX512_MASK);              \
            _mm512_storeu_si512((__m512i *)(result), result_vec);                                    \
        }                                                                                            \
    } while (0)

#define __SUB_N_K(result, a, b, b_in, b_out, k, remaining)                                           \
    do                                                                                               \
    {                                                                                                \
        __m512i a_vec = _mm512_mask_loadu_epi64(AVX512_ZEROS, k, (__m512i *)(a));                    \
        __m512i b_vec = _mm512_mask_loadu_epi64(AVX512_ZEROS, k, (__m512i *)(b));                    \
        __m512i result_vec = _mm512_sub_epi64(a_vec, b_vec);                                         \
        __mmask16 b_mask = _mm512_cmpgt_epu64_mask(b_vec, a_vec);                                    \
        b_out = b_mask >> remaining;                                                                 \
        b_mask <<= 1;                                                                                \
        b_mask = _mm512_kor(b_mask, b_in);                                                           \
        __m512i result_vec_new = _mm512_mask_add_epi64(result_vec, b_mask, result_vec, AVX512_MASK); \
        b_mask = _mm512_cmpgt_epu64_mask(result_vec_new, result_vec);                                \
        result_vec = result_vec_new;                                                                 \
        _mm512_mask_storeu_epi64((__m512i *)(result), k, result_vec);                                \
        if (unlikely(_mm512_mask2int(b_mask)))                                                       \
        {                                                                                            \
            b_mask <<= 1;                                                                            \
            __mmask16 m = _mm512_cmpeq_epu64_mask(result_vec, AVX512_ZEROS);                         \
            b_mask = b_mask + m;                                                                     \
            b_out = _mm512_kor(b_out, (b_mask >> (remaining + 1)));                                  \
            m = _mm512_kxor(b_mask, m);                                                              \
            result_vec = _mm512_mask_add_epi64(result_vec, m, result_vec, AVX512_MASK);              \
            _mm512_mask_storeu_epi64((__m512i *)(result), k, result_vec);                            \
        }                                                                                            \
    } while (0)

/***************************************** Approximate Variants *****************************************/

#define __ADD_N_4_APPROX(result, a, b, c_out)                                            \
    do                                                                                   \
    {                                                                                    \
        __m256i a_vec = _mm256_loadu_si256((__m256i *)(a));                              \
        __m256i b_vec = _mm256_loadu_si256((__m256i *)(b));                              \
        __m256i result_vec = _mm256_add_epi64(a_vec, b_vec);                             \
        __mmask16 c_mask = _mm256_cmplt_epu64_mask(result_vec, a_vec);                   \
        c_out = c_mask >> 3;                                                             \
        c_mask <<= 1;                                                                    \
        result_vec = _mm256_mask_sub_epi64(result_vec, c_mask, result_vec, AVX256_MASK); \
        _mm256_storeu_si256((__m256i *)(result), result_vec);                            \
    } while (0)

#define __ADD_N_8_APPROX(result, a, b, c_in, c_out)                                      \
    do                                                                                   \
    {                                                                                    \
        __m512i a_vec = _mm512_loadu_si512((__m512i *)(a));                              \
        __m512i b_vec = _mm512_loadu_si512((__m512i *)(b));                              \
        __m512i result_vec = _mm512_add_epi64(a_vec, b_vec);                             \
        __mmask16 c_mask = _mm512_cmplt_epu64_mask(result_vec, a_vec);                   \
        c_out = c_mask >> 7;                                                             \
        c_mask <<= 1;                                                                    \
        c_mask = _mm512_kor(c_mask, c_in);                                               \
        result_vec = _mm512_mask_sub_epi64(result_vec, c_mask, result_vec, AVX512_MASK); \
        _mm512_storeu_si512((__m512i *)(result), result_vec);                            \
    } while (0)

#define __SUB_N_4_APPROX(result, a, b)                                                   \
    do                                                                                   \
    {                                                                                    \
        __m256i a_vec = _mm256_loadu_si256((__m256i *)(a));                              \
        __m256i b_vec = _mm256_loadu_si256((__m256i *)(b));                              \
        __m256i result_vec = _mm256_sub_epi64(a_vec, b_vec);                             \
        __mmask16 b_mask = _mm256_cmpgt_epu64_mask(b_vec, a_vec);                        \
        b_mask <<= 1;                                                                    \
        result_vec = _mm256_mask_add_epi64(result_vec, b_mask, result_vec, AVX256_MASK); \
        _mm256_storeu_epi64((__m256i *)(result), result_vec);                            \
    } while (0)

#define __SUB_N_8_APPROX(result, a, b, b_in, b_out)                                      \
    do                                                                                   \
    {                                                                                    \
        __m512i a_vec = _mm512_loadu_si512((__m512i *)(a));                              \
        __m512i b_vec = _mm512_loadu_si512((__m512i *)(b));                              \
        __m512i result_vec = _mm512_sub_epi64(a_vec, b_vec);                             \
        __mmask16 b_mask = _mm512_cmpgt_epu64_mask(b_vec, a_vec);                        \
        b_out = b_mask >> 7;                                                             \
        b_mask <<= 1;                                                                    \
        b_mask = _mm512_kor(b_mask, b_in);                                               \
        result_vec = _mm512_mask_add_epi64(result_vec, b_mask, result_vec, AVX512_MASK); \
        _mm512_storeu_si512((__m512i *)(result), result_vec);                            \
    } while (0)

/***************************************** Function Prototypes *****************************************/

void dot_add_n(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_add_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_sub_n(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_sub_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);

unsigned long dot_add_words(uint64_t *result, const uint64_t *a, const uint64_t *b, int n);
unsigned long dot_sub_words(uint64_t *result, const uint64_t *a, const uint64_t *b, int n);
#endif // DOT_H
