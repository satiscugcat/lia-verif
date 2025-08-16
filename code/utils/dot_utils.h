#ifndef DOT_UTILS_H
#define DOT_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <immintrin.h>
#include <inttypes.h>
#include <errno.h>

// Define the aligned data types
typedef uint64_t aligned_uint64 __attribute__((aligned(64)));      // Define an aligned uint64_t
typedef uint64_t *aligned_uint64_ptr __attribute__((aligned(64))); // Define an aligned pointer to uint64_t

// Declare threshold for borrow propagation and dot_limb digits
extern aligned_uint64 DOT_LIMB_DIGITS; // 10^18, used for carry/borrow-propagation, as we're using 64-bit integers; mostly saturated

// A structure to store the dot_limbs
typedef struct
{
    aligned_uint64_ptr dot_limbs; // Pointer to the dot_limbs
    bool sign;                    // Sign of the number
    size_t size;                  // Size of the dot_limbs
    bool carry;                   // Carry flag
} dot_limb_t;

// Declare the SIMD constants
extern __m512i AVX512_ZEROS;           // 0 as chunk of 8 64-bit integers
extern __m512i AVX512_ONES;            // 1 as chunk of 8 64-bit integers
extern __m512i AVX512_DOT_LIMB_DIGITS; // 10^18 as chunk of 8 64-bit integers

#define unlikely(expr) __builtin_expect(!!(expr), 0) // unlikely branch
#define likely(expr) __builtin_expect(!!(expr), 1)   // likely branch

// Memory pool functions
void init_memory_pool();
void *memory_pool_alloc(size_t size);
void memory_pool_free(void *ptr);
void destroy_memory_pool();

/**
 * @brief Allocates dot_limb_t structure, with fixed alignment of 64
 *
 * @param size The number of dot_limbs to allocate
 * @return dot_limb_t* The pointer to the allocated memory
 * @note The memory should be freed using memory_pool_free
 */
dot_limb_t *dot_limb_t_alloc(size_t size);

/**
 * @brief Reallocates memory with fixed alignment of 64
 *
 * @param dot_limb The pointer to the memory to reallocate
 * @param new_size The new size of the memory
 * @return dot_limb_t* The pointer to the reallocated memory
 * @note The old memory is freed
 */
dot_limb_t *dot_limb_t_realloc(dot_limb_t *dot_limb, size_t new_size);

/**
 * @brief Frees memory allocated by dot_limb_t_alloc
 *
 * @param dot_limb The pointer to the dot_limb_t structure to free
 * @return void
 */
void dot_limb_t_free(dot_limb_t *dot_limb);

/**
 * @brief Converts a large number represented by a dot_limb_t structure into a hex-string.
 *
 * @param num The number to convert
 * @return char* The string representation of the number
 */
char *dot_limb_get_str(const dot_limb_t *num);

/**
 * @biref Internal function to convert a dot_limb_t structure into a hex-string, usually called by dot_limb_get_str
 *
 * @param num The number to convert
 * @param str The string to store the result
 * @return void
 */
void __get_str(const dot_limb_t *num, char *str);

/**
 * @brief Converts a hex-string representing a large number into a dot_limb_t structure.
 *
 * This function takes a hex-string representing a large number and converts it into a dot_limb_t structure.
 * The dot_limbs are allocated with a fixed alignment of 64 bytes.
 *
 * @param str The number as a string.
 * @return A pointer to the dot_limb_t structure representing the large number.
 */
dot_limb_t *dot_limb_set_str(const char *str);

/**
 * @brief Internal function to convert a hex-string into a dot_limb_t structure, usually called by dot_limb_set_str
 *
 * @param aligned_uint64_ptr digits The digits of the number
 * @param size_t n The number of digits
 * @param dot_limb_t *num The number to convert
 *
 * @return void
 */
void __set_str(aligned_uint64_ptr digits, size_t n, dot_limb_t *num);

// /**
//  * @brief Adjusts the sizes of two dot_limb_t structures to be equal.
//  *
//  * This function takes two dot_limb_t structures and adjusts their sizes to be equal by reallocating memory
//  * and padding with zeros if necessary.
//  *
//  * @param num1 A pointer to the first dot_limb_t structure.
//  * @param num2 A pointer to the second dot_limb_t structure.
//  */
void dot_limb_t_adjust_sizes(dot_limb_t *num1, dot_limb_t *num2);

#endif // DOT_UTILS_H