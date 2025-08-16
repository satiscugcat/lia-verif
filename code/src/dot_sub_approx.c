#include "dot_utils.h"
#include "dot.h"

extern __m512i AVX512_ZEROS; // AVX512 vector of zeros
extern __m256i AVX256_ZEROS; // AVX256 vector of zeros
extern __m128i AVX128_ZEROS; // AVX128 vector of zeros
extern __m512i AVX512_MASK;  // AVX512 vector of 64-bit mask
extern __m256i AVX256_MASK;  // AVX256 vector of 64-bit mask
extern __m128i AVX128_MASK;  // AVX128 vector of 64-bit mask

void dot_sub_approx_256(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    // swap a and b if a < b
    int cmp = 0;
    for (int i = 3; i >= 0; --i)
    {
        if (a->dot_limbs[i] > b->dot_limbs[i])
        {
            cmp = 1;
            break;
        }
        else if (a->dot_limbs[i] < b->dot_limbs[i])
        {
            cmp = -1;
            break;
        }
    }
    if (cmp == -1)
    {
        dot_limb_t *temp = a;
        a = b;
        b = temp;
        result->sign = 1;
    }
    else if (cmp == 0)
    {
        result->size = 1;
        result->dot_limbs[0] = 0;
        return;
    }
    __SUB_N_4_APPROX((result->dot_limbs), (a->dot_limbs), (b->dot_limbs));
}

void __sub_n_approx(dot_limb_t *result, dot_limb_t *x, dot_limb_t *y)
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
    for (int i = 0; i < n; i += 8)
    {
        __SUB_N_8_APPROX((res_ptr + i), (x_ptr + i), (y_ptr + i), b_in, b_out);
        b_in = b_out;
    }
}

void dot_sub_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b)
{
    int n = a->size;

    if (n <= 4)
    {
        dot_sub_approx_256(result, a, b);
    }
    else
    {
        __sub_n_approx(result, a, b);
    }
}