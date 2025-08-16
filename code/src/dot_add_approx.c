#include "dot_utils.h"
#include "dot.h"

extern __m512i AVX512_ZEROS; // AVX512 vector of zeros
extern __m256i AVX256_ZEROS; // AVX256 vector of zeros
extern __m128i AVX128_ZEROS; // AVX128 vector of zeros
extern __m512i AVX512_MASK;  // AVX512 vector of 64-bit mask
extern __m256i AVX256_MASK;  // AVX256 vector of 64-bit mask
extern __m128i AVX128_MASK;  // AVX128 vector of 64-bit mask

void __add_n_256_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    __mmask16 c_out = 0;
    __ADD_N_4_APPROX((result->dot_limbs), (a->dot_limbs), (b->dot_limbs), c_out);
    result->carry = c_out;
}

void __add_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    uint64_t *res_ptr = result->dot_limbs;
    uint64_t *a_ptr = a->dot_limbs;
    uint64_t *b_ptr = b->dot_limbs;
    const int n = a->size;
    __mmask16 c_in = 0, c_out = 0;
    for (int i = 0; i < n; i += 8)
    {
        __ADD_N_8_APPROX((res_ptr + i), (a_ptr + i), (b_ptr + i), c_in, c_out);
        c_in = c_out;
    }
    result->carry = c_out;
}

void dot_add_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    int n = a->size;
    if (likely(n > 4))
    {
        __add_n_approx(result, a, b);
    }
    else
    {
        __add_n_256_approx(result, a, b);
    }
}