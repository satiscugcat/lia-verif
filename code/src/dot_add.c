#include "dot_utils.h"
#include "dot.h"
#include <assert.h>

extern __m512i AVX512_ZEROS; // AVX512 vector of zeros
extern __m256i AVX256_ZEROS; // AVX256 vector of zeros
extern __m128i AVX128_ZEROS; // AVX128 vector of zeros
extern __m512i AVX512_MASK;  // AVX512 vector of 64-bit mask
extern __m256i AVX256_MASK;  // AVX256 vector of 64-bit mask
extern __m128i AVX128_MASK;  // AVX128 vector of 64-bit mask



void dot_add_n(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    uint64_t *res_ptr = result->dot_limbs;
    uint64_t *a_ptr = a->dot_limbs;
    uint64_t *b_ptr = b->dot_limbs;
    const int n = a->size;
    __mmask16 c_in = 0, c_out = 0;

    // Process limbs in chunks of 8
    int i;
    for (i = 0; i <= n - 8; i += 8)
    {
        __ADD_N_8((res_ptr + i), (a_ptr + i), (b_ptr + i), c_in, c_out);
        c_in = c_out;
    }

    // Handle remaining limbs (if any)
    if (unlikely(i < n))
    {
        // Create a mask for the remaining limbs
        __mmask16 remaining = n - i;
        __mmask16 k = (1ULL << remaining) - 1; // Mask with 'remaining' number of 1s
        remaining--;

        // Process remaining limbs using __ADD_N_K
        __ADD_N_K((res_ptr + i), (a_ptr + i), (b_ptr + i), c_in, c_out, k, remaining);
    }
    result->carry = c_out;
}

unsigned long dot_add_words(uint64_t *result, const uint64_t *a, const uint64_t *b, int n)
{
    assert(n >= 0);
    if (n <= 0)
        return (unsigned long)0; // No limbs to add
    AVX512_ZEROS = _mm512_setzero_si512();
    AVX512_MASK = _mm512_set1_epi64(0xFFFFFFFFFFFFFFFF);
    __mmask16 c_in = 0, c_out = 0;
    // Process limbs in chunks of 8
    int i;
    for (i = 0; i <= n - 8; i += 8)
    {
        __ADD_N_8((result + i), (a + i), (b + i), c_in, c_out);
        c_in = c_out;
    }

    // Handle remaining limbs (if any)
    if (unlikely(i < n))
    {
        // Create a mask for the remaining limbs
        __mmask16 remaining = n - i;
        __mmask16 k = (1ULL << remaining) - 1; // Mask with 'remaining' number of 1s
        remaining--;

        // Process remaining limbs using __ADD_N_K
        __ADD_N_K((result + i), (a + i), (b + i), c_in, c_out, k, remaining);
    }
    return (unsigned long)!!c_out; // Return carry out
}
