#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <immintrin.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <ctype.h>
#include "dot_utils.h"

#define LIMB_BITS 64             // Number of hex digits in each dot_limb
#define bits 4                   // Number of bits in each hex digit
#define MEMORY_POOL_SIZE 1 << 25 // 1 GB memory pool

// Define the aligned data types
// typedef uint64_t aligned_uint64 __attribute__((aligned(64)));      // Define an aligned uint64_t
// typedef uint64_t *aligned_uint64_ptr __attribute__((aligned(64))); // Define an aligned pointer to uint64_t
typedef uint64_t aligned_uint64;      // Define an aligned uint64_t
typedef uint64_t *aligned_uint64_ptr; // Define an aligned pointer to uint64_t

// Declare the SIMD constants
__m512i AVX512_ZEROS; // AVX512 vector of zeros
__m256i AVX256_ZEROS; // AVX256 vector of zeros
__m128i AVX128_ZEROS; // AVX128 vector of zeros
__m512i AVX512_MASK;  // AVX512 vector of 52-bit mask
__m256i AVX256_MASK;  // AVX256 vector of 52-bit mask
__m128i AVX128_MASK;  // AVX128 vector of 52-bit mask

// Memory pool variables
static uint8_t *memory_pool = NULL;
static size_t memory_pool_offset = 0;
static size_t memory_pool_free_count = 0;

void init_memory_pool()
{
    memory_pool = (uint8_t *)malloc(MEMORY_POOL_SIZE);
    if (memory_pool == NULL)
    {
        perror("Memory allocation failed for memory pool\n");
        exit(EXIT_FAILURE);
    }
    memory_pool_offset = 0;
    AVX512_ZEROS = _mm512_setzero_si512();
    AVX256_ZEROS = _mm256_setzero_si256();
    AVX128_ZEROS = _mm_setzero_si128();

    AVX512_MASK = _mm512_set1_epi64(0xFFFFFFFFFFFFFFFF);
    AVX256_MASK = _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF);
    AVX128_MASK = _mm_set1_epi64x(0xFFFFFFFFFFFFFFFF);
}

void *memory_pool_alloc(size_t size)
{
    // Align the memory offset to 64 bytes
    size_t aligned_offset = memory_pool_offset++;

    if (aligned_offset + size > MEMORY_POOL_SIZE)
    {
        perror("Memory pool exhausted\n");
        exit(EXIT_FAILURE);
    }

    void *ptr = memory_pool + aligned_offset;
    memory_pool_offset = aligned_offset + size;
    return ptr;
}

void memory_pool_free(void *ptr)
{
    // if the offset is greater than 512MB, reset the memory pool
    if (memory_pool_offset > (MEMORY_POOL_SIZE >> 1))
    {
        memory_pool_offset = 0;
        memory_pool_free_count++;
        memset(memory_pool, 0, MEMORY_POOL_SIZE);
    }
}

void destroy_memory_pool()
{
    if (memory_pool != NULL)
    {
        _mm_free(memory_pool);
        memory_pool = NULL;
        memory_pool_offset = 0;
    }
}

dot_limb_t *dot_limb_t_alloc(size_t size)
{
    // check if the size is 0
    if (size == 0)
    {
        return NULL;
    }

    dot_limb_t *dot_limb = (dot_limb_t *)memory_pool_alloc(sizeof(dot_limb_t));
    // check if the memory allocation failed
    if (dot_limb == NULL)
    {
        perror("Memory allocation failed for dot_limb_t structure\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory with fixed alignment of 64
    dot_limb->dot_limbs = (uint64_t *)memory_pool_alloc(size * sizeof(uint64_t));
    // check if the memory allocation failed
    if (dot_limb->dot_limbs == NULL)
    {
        perror("Memory allocation failed for dot_limb_t_alloc\n");
        exit(EXIT_FAILURE);
    }

    dot_limb->size = size;
    dot_limb->sign = false; // Initialize sign to false (positive)

    return dot_limb;
}

dot_limb_t *dot_limb_t_realloc(dot_limb_t *dot_limb, size_t new_size)
{
    // Check if the dot_limb is NULL
    if (dot_limb == NULL)
    {
        return NULL;
    }

    // Check if the size is the same
    if (dot_limb->size == new_size)
    {
        return dot_limb;
    }

    // Check if the new size is 0
    if (new_size == 0)
    {
        memory_pool_free(dot_limb->dot_limbs);
        dot_limb->dot_limbs = NULL;
        dot_limb->size = 0;
        return dot_limb;
    }

    // Allocate new memory with fixed alignment of 64
    uint64_t *new_dot_limbs = (uint64_t *)memory_pool_alloc(new_size * sizeof(uint64_t));
    if (new_dot_limbs == NULL)
    {
        return NULL;
    }

    // Copy old data to new memory, preserving least significant digits at lower indices
    size_t copy_size = dot_limb->size < new_size ? dot_limb->size : new_size;
    memcpy(new_dot_limbs, dot_limb->dot_limbs, copy_size * sizeof(uint64_t));

    // If growing, append zeros at the end (higher indices)
    if (new_size > dot_limb->size)
    {
        memset(new_dot_limbs + copy_size, 0, (new_size - copy_size) * sizeof(uint64_t));
    }

    // Free the old memory and update dot_limb structure
    memory_pool_free(dot_limb->dot_limbs);
    dot_limb->dot_limbs = new_dot_limbs;
    dot_limb->size = new_size;

    return dot_limb;
}

void dot_limb_t_free(dot_limb_t *dot_limb)
{
    if (dot_limb != NULL)
    {
        if (dot_limb->dot_limbs != NULL)
        {
            memory_pool_free(dot_limb->dot_limbs);
        }
        memory_pool_free(dot_limb);
    }
    dot_limb = NULL;
}

void __get_str(const dot_limb_t *num, char *str)
{
    char *sp = str;
    const char *digits = "0123456789abcdef";
    if (num->sign)
    {
        *sp++ = '-';
    }
    size_t num_dot_limbs = num->size;
    unsigned char mask = 0xF; // Assuming 4 bits per digit (hex)
    bool leading_zeros = true;

    // If there's a carry, prepend it as the most significant digit
    if (num->carry == 1)
    {
        *sp++ = digits[1];     // Prepend '1' for the carry
        leading_zeros = false; // Since we added a non-zero digit
    }

    for (int i = num_dot_limbs - 1; i >= 0; i--)
    {
        for (int shift = LIMB_BITS - bits; shift >= 0; shift -= bits)
        {
            unsigned char digit = (num->dot_limbs[i] >> shift) & mask;
            // Skip leading zeros until we find a non-zero digit
            if (digit != 0 || !leading_zeros)
            {
                *sp++ = digits[digit];
                leading_zeros = false;
            }
        }
    }
    // If all digits were zero (and no carry), output a single '0'
    if (leading_zeros)
    {
        *sp++ = '0';
    }
    *sp = '\0';
}

char *dot_limb_get_str(const dot_limb_t *num)
{
    if (num == NULL || num->size == 0 || num->dot_limbs == NULL)
    {
        return NULL;
    }

    if (num->size == 1 && num->dot_limbs[0] == 0)
    {
        char *zero = (char *)memory_pool_alloc(2); // Allocate for "0\0"
        if (zero == NULL)
        {
            perror("Memory allocation failed for string\n");
            exit(EXIT_FAILURE);
        }
        strcpy(zero, "0");
        return zero;
    }
    // Calculate string length: each dot_limb produces LIMB_BITS/bits digits, plus sign and null terminator
    size_t hex_len = (num->size * LIMB_BITS) / bits + 2; // +2 for sign and '\0'

    char *str = (char *)memory_pool_alloc(hex_len);
    if (str == NULL)
    {
        perror("Memory allocation failed for string\n");
        exit(EXIT_FAILURE);
    }

    __get_str(num, str);

    // If the string is just a negative sign, add a zero
    if (str[0] == '-' && str[1] == '\0')
    {
        str[1] = '0';
        str[2] = '\0';
    }

    return str;
}

void __set_str(aligned_uint64_ptr digits, size_t n, dot_limb_t *num)
{
    size_t num_dot_limbs = num->size;
    aligned_uint64 dot_limb = 0;
    unsigned shift = 0;
    int dot_limb_index = 0;

    for (size_t i = 0; i < n;)
    {
        for (shift = 0; shift < LIMB_BITS && i < n; shift += bits)
        {
            dot_limb |= (aligned_uint64)digits[i] << shift;
            i++;
        }
        num->dot_limbs[dot_limb_index++] = dot_limb;
        dot_limb = 0;
    }
    while (dot_limb_index < num_dot_limbs)
    {
        num->dot_limbs[dot_limb_index++] = 0;
    }
    num->size = num_dot_limbs;
}

dot_limb_t *dot_limb_set_str(const char *str)
{
    // Check if the string is NULL or empty
    if (str == NULL || strlen(str) == 0)
    {
        return NULL;
    }

    // Allocate temporary memory for hex-string to digit conversion
    size_t hex_len = strlen(str);
    aligned_uint64_ptr digits = (uint64_t *)memory_pool_alloc(hex_len * sizeof(uint64_t));
    if (digits == NULL)
    {
        perror("Memory allocation failed for digits\n");
        exit(EXIT_FAILURE);
    }

    // Extract sign and omit any whitespace
    bool sign = false;
    if (str[0] == '-')
    {
        sign = true;
    }

    const char *sp = str + (sign ? 1 : 0);
    while (isspace(*sp))
    {
        sp++;
    }

    // Calculate actual length after skipping sign and whitespace
    size_t actual_len = strlen(sp);

    // Convert the hex-string to digits in reverse order
    for (size_t i = 0; i < actual_len; i++)
    {
        char c = sp[actual_len - 1 - i]; // Read characters from end to start
        // If the character is between 0-9
        if (c >= '0' && c <= '9')
        {
            digits[i] = c - '0';
        }
        // If the character is between A-F
        else if (c >= 'A' && c <= 'F')
        {
            digits[i] = c - 'A' + 10;
        }
        // If the character is between a-f
        else if (c >= 'a' && c <= 'f')
        {
            digits[i] = c - 'a' + 10;
        }
        else
        {
            perror("Invalid character in hex-string\n");
            exit(EXIT_FAILURE);
        }
    }

    size_t num_dot_limbs = (actual_len * bits + LIMB_BITS - 1) / LIMB_BITS; // ceil(actual_len * bits / LIMB_BITS)
    dot_limb_t *num = dot_limb_t_alloc(num_dot_limbs);
    if (num == NULL)
    {
        perror("Memory allocation failed for num\n");
        exit(EXIT_FAILURE);
    }
    num->size = num_dot_limbs;
    num->sign = sign;
    num->carry = false; // Initialize carry to false

    __set_str(digits, actual_len, num);
    memory_pool_free(digits); // Free the temporary digits array
    return num;
}
void dot_limb_t_adjust_sizes(dot_limb_t *num1, dot_limb_t *num2)
{
    if (num1->size == num2->size)
    {
        return;
    }

    size_t max_size = num1->size > num2->size ? num1->size : num2->size;

    if (num1->size < max_size)
    {
        num1 = dot_limb_t_realloc(num1, max_size);
        if (num1->dot_limbs == NULL)
        {
            perror("Memory reallocation failed for num1\n");
            exit(1);
        }
        num1->size = max_size;
    }

    if (num2->size < max_size)
    {
        num2 = dot_limb_t_realloc(num2, max_size);
        if (num2->dot_limbs == NULL)
        {
            perror("Memory reallocation failed for num2\n");
            exit(1);
        }
        num2->size = max_size;
    }
}