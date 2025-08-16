#include "dot.h"
#include "dot_utils.h"
#include <assert.h>

extern __m512i AVX512_ZEROS; // AVX512 vector of zeros
extern __m256i AVX256_ZEROS; // AVX256 vector of zeros
extern __m128i AVX128_ZEROS; // AVX128 vector of zeros
extern __m512i AVX512_MASK;  // AVX512 vector of 64-bit mask
extern __m256i AVX256_MASK;  // AVX256 vector of 64-bit mask
extern __m128i AVX128_MASK;  // AVX128 vector of 64-bit mask

void dot_sub_n(dot_limb_t *result, dot_limb_t *x, dot_limb_t *y)
{
    int n = x->size;
    // swap x and y if x < y
    int cmp = 0;
    for (int i = n - 1; i >= 0; --i)
    {
        if (x->dot_limbs[i] > y->dot_limbs[i])
        {
            cmp = 1;
            break;
        }
        else if (x->dot_limbs[i] < y->dot_limbs[i])
        {
            cmp = -1;
            break;
        }
    }
    if (cmp == -1)
    {
        dot_limb_t *temp = x;
        x = y;
        y = temp;
        result->sign = 1;
    }
    else if (cmp == 0)
    {
        result->size = 1;
        result->dot_limbs[0] = 0;
        return;
    }

    uint64_t *res_ptr = result->dot_limbs;
    uint64_t *x_ptr = x->dot_limbs;
    uint64_t *y_ptr = y->dot_limbs;

    __mmask16 b_in = 0, b_out = 0;
    int i;
    for (i = 0; i < n - 8; i += 8)
    {
        __SUB_N_8((res_ptr + i), (x_ptr + i), (y_ptr + i), b_in, b_out);
        b_in = b_out;
    }

    // Handle remaining limbs (if any)
    if (i < n)
    {
        // Create a mask for the remaining limbs
        int remaining = n - i;
        __mmask16 k = (1ULL << remaining) - 1; // Mask with 'remaining' number of 1s

        // Process remaining limbs using __SUB_N_K
        __SUB_N_K((res_ptr + i), (x_ptr + i), (y_ptr + i), b_in, b_out, k, (remaining - 1));
    }
}

/* unsigned subtraction of b from a, a must be larger than b. */
unsigned long dot_sub_words(uint64_t *result, const uint64_t *x, const uint64_t *y, int n)
{
    assert(n >= 0);
    if (n <= 0)
        return (unsigned long)0; // No limbs to subtract

    AVX512_ZEROS = _mm512_setzero_si512();
    AVX512_MASK = _mm512_set1_epi64(0xFFFFFFFFFFFFFFFF);
    __mmask16 b_in = 0, b_out = 0;
    int i;
    for (i = 0; i < n - 8; i += 8)
    {
        __SUB_N_8((result + i), (x + i), (y + i), b_in, b_out);
        b_in = b_out;
    }

    // Handle remaining limbs (if any)
    if (unlikely(i < n))
    {
        // Create a mask for the remaining limbs
        __mmask16 remaining = n - i;
        __mmask16 k = (1ULL << remaining) - 1; // Mask with 'remaining' number of 1s
        // Process remaining limbs using __SUB_N_K
        remaining--;
        __SUB_N_K((result + i), (x + i), (y + i), b_in, b_out, k, remaining);
    }
    return (unsigned long)!!b_out; // Return the borrow mask
}