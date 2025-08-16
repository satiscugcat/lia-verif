#ifndef DOTLIB_H
#define DOTLIB_H

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

typedef uint64_t aligned_uint64 __attribute__((aligned(64)));
typedef uint64_t *aligned_uint64_ptr __attribute__((aligned(64)));

typedef struct
{
    aligned_uint64_ptr dot_limbs; // Pointer to the limbs
    bool sign;                    // Sign of the number (true for negative)
    size_t size;                  // Number of limbs
    bool carry;                   // Carry flag
} dot_limb_t;

// Arithmetic operations
void dot_add_n(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_sub_n(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_add_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);
void dot_sub_n_approx(dot_limb_t *result, dot_limb_t *a, dot_limb_t *b);

unsigned long dot_add_words(uint64_t *result, const uint64_t *a, const uint64_t *b, int n);
unsigned long dot_sub_words(uint64_t *result, const uint64_t *a, const uint64_t *b, int n);

// Memory and utility functions
dot_limb_t *dot_limb_t_alloc(size_t size);
void dot_limb_t_free(dot_limb_t *dot_limb);
dot_limb_t *dot_limb_t_realloc(dot_limb_t *dot_limb, size_t new_size);
char *dot_limb_get_str(const dot_limb_t *num);
dot_limb_t *dot_limb_set_str(const char *str);
void dot_limb_t_adjust_sizes(dot_limb_t *num1, dot_limb_t *num2);

// Initialize memory pool
void init_memory_pool(void);
void destroy_memory_pool(void);

#endif // DOTLIB_H