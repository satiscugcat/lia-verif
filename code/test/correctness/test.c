#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "dotlib.h"

#define RANDOM_ITERATIONS 100000 // Number of random test case iterations
#define SPECIAL_ITERATIONS 1000  // Number of special test case iterations
#define CHUNK 655360             // Chunk size for reading the file
#define MAX_FAILURES 1000        // Maximum number of failures to track

// Function to trim leading zeros and whitespace characters
char *trim_leading_zeros_and_whitespace(char *str)
{
    while (*str == '0' || isspace(*str))
    {
        str++;
    }
    return str;
}

// Function to trim trailing newline characters
void trim_trailing_newline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

bool check_result(char *result, char *result_gmp, int result_size)
{
    // Check if both results start with a negative symbol
    bool result_negative = (result[0] == '-');
    bool result_gmp_negative = (result_gmp[0] == '-');

    // Adjust pointers to skip the negative symbol for comparison
    if (result_negative)
        result++;
    if (result_gmp_negative)
        result_gmp++;

    // Trim leading zeros and whitespace characters from both strings
    result = trim_leading_zeros_and_whitespace(result);
    result_gmp = trim_leading_zeros_and_whitespace(result_gmp);

    // Trim trailing newline characters from both strings
    trim_trailing_newline(result);
    trim_trailing_newline(result_gmp);

    // convert the result to lower case
    for (int i = 0; i < result_size; i++)
    {
        result[i] = tolower(result[i]);
        result_gmp[i] = tolower(result_gmp[i]);
    }

    // Check if the lengths of the adjusted strings are equal
    if (strlen(result) != strlen(result_gmp))
    {
        printf("The two sums are not equal, lengths are different\n");
        printf("Length of result = %ld, length of result_gmp = %ld\n", strlen(result), strlen(result_gmp));
        printf("result = %s\n result_gmp = %s\n", result, result_gmp);
        return false;
    }

    // Compare the adjusted strings character by character
    for (size_t i = 0; i < strlen(result); i++)
    {
        if (result[i] != result_gmp[i])
        {
            printf("The two sums are not equal\n");
            printf("Mismatch at index %ld\n", i);
            printf("result[%ld] = %c, result_gmp[%ld] = %c\n", i, result[i], i, result_gmp[i]);
            printf("result = %s\nresult_gmp = %s\n", result, result_gmp);
            return false;
        }
    }

    // If both results were negative, ensure the negative symbol is accounted for
    if (result_negative != result_gmp_negative)
    {
        printf("The two results are not equal, one is negative and the other is not\n");
        return false;
    }

    return true;
}

void skip_first_line(gzFile file)
{
    char buffer[CHUNK];
    if (gzgets(file, buffer, sizeof(buffer)) == NULL)
    {
        perror("Error reading header line");
        gzclose(file);
        exit(EXIT_FAILURE);
    }
}
// Function to open a gzip file and handle errors
gzFile open_gzfile(const char *filename, const char *mode)
{
    gzFile file = gzopen(filename, mode);
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return file;
}

/*
    op: 0 -> addition,
        1 -> subtraction
        2 -> approximated addition
        3 -> approximated subtraction
    case_type: 0 -> random cases
               1 -> special cases
*/
void run_test(int op, int NUM_BITS, int case_type)
{
    const char *file_type = (case_type == 0) ? "random" : "special";
    int iterations = (case_type == 0) ? RANDOM_ITERATIONS : SPECIAL_ITERATIONS;

    printf("Running %s test with %d bits on %s test cases\n",
           op == 0 ? "addition" : (op == 1 ? "subtraction" : (op == 2 ? "approximated addition" : "approximated subtraction")),
           NUM_BITS, file_type);

    char test_filename[100];
    snprintf(test_filename, sizeof(test_filename), "./cases/%s/%d/%s.csv.gz",
             (op % 2) ? "sub" : "add", NUM_BITS, file_type);

    // open the test file
    gzFile test_file = open_gzfile(test_filename, "rb");

    // skip the first line, header
    skip_first_line(test_file);

    // Track failed test cases
    int failed_cases[MAX_FAILURES];
    int total_failures = 0;
    int total_tests = 0;

    // Read test cases
    for (int i = 0; i < iterations; i++)
    {
        init_memory_pool();
        // Read the next line
        char buffer[CHUNK];
        if (gzgets(test_file, buffer, sizeof(buffer)) == NULL)
        {
            if (gzeof(test_file))
            {
                break; // EOF
            }
            else
            {
                perror("Error reading line");
                gzclose(test_file);
                exit(EXIT_FAILURE);
            }
        }

        // Parse the test case
        char *a_str = strtok(buffer, ",");
        char *b_str = strtok(NULL, ",");
        char *result_str = strtok(NULL, ",");

        if (a_str == NULL || b_str == NULL || result_str == NULL)
        {
            fprintf(stderr, "Error parsing line: %s\n", buffer);
            gzclose(test_file);
            exit(EXIT_FAILURE);
        }

        int n_1 = strlen(a_str);
        int n_2 = strlen(b_str);

        // convert a and b into dot_limbs
        dot_limb_t *a, *b;
        a = dot_limb_set_str(a_str);
        b = dot_limb_set_str(b_str);

        // adjust the sizes of a and b
        dot_limb_t_adjust_sizes(a, b);
        int n = a->size;

        dot_limb_t *s = dot_limb_t_alloc(n);

        /***** Start of operation *****/

        typedef void (*dot_operation_func)(dot_limb_t *, dot_limb_t *, dot_limb_t *);
        // if op == 0, addition, 1 == subtraction, 2 == approximated addition, 3 == approximated subtraction
        dot_operation_func func = op == 0 ? dot_add_n : (op == 1 ? dot_sub_n : (op == 2 ? dot_add_n_approx : dot_sub_n_approx));
        // Perform the operation
        func(s, a, b);

        /***** End of operation *****/

        char *sum_str = dot_limb_get_str(s);
        int str_len = strlen(sum_str);

        total_tests++;

        // verify the converted string with result
        if (!check_result(sum_str, result_str, str_len))
        {
            printf("Test case failed, at iteration %d\n", i);
            printf("a = %s\n", a_str);
            printf("b = %s\n", b_str);
            // print the result
            printf("result: ");
            for (int j = 0; j < n; j++)
            {
                printf("%016lx ", s->dot_limbs[j]);
            }
            printf("\n");
            printf("sum_str = %s\n", sum_str);

            // Record this failure
            if (total_failures < MAX_FAILURES)
            {
                failed_cases[total_failures] = i;
            }
            total_failures++;
        }

        dot_limb_t_free(a);
        dot_limb_t_free(b);
        dot_limb_t_free(s);
        destroy_memory_pool();
    }
    gzclose(test_file);

    // Print summary of results
    printf("\n===== TEST SUMMARY =====\n");
    printf("Total test cases executed: %d\n", total_tests);
    printf("Total test cases failed: %d\n", total_failures);
    printf("Total test cases passed: %d\n", total_tests - total_failures);

    if (total_failures > 0)
    {
        printf("\nFailed test case numbers: ");
        int display_count = total_failures < MAX_FAILURES ? total_failures : MAX_FAILURES;
        for (int i = 0; i < display_count; i++)
        {
            printf("%d", failed_cases[i]);
            if (i < display_count - 1)
            {
                printf(", ");
            }
        }
        if (total_failures > MAX_FAILURES)
        {
            printf(", ... (and %d more)", total_failures - MAX_FAILURES);
        }
        printf("\n");

        // Exit with failure status if any tests failed
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <operation> <number_of_bits> <case_type>\n", argv[0]);
        fprintf(stderr, "operation: 0 for addition, 1 for subtraction, 2 for approximated addition, 3 for approximated subtraction\n");
        fprintf(stderr, "number_of_bits: number of bits for the test cases\n");
        fprintf(stderr, "case_type: 0 for random test cases, 1 for special test cases\n");
        fprintf(stderr, "Example: %s 0 256 1\n", argv[0]);
        fprintf(stderr, "This will run addition tests on special test cases with 256 bits\n");
        fprintf(stderr, "Example: %s 3 512 0\n", argv[0]);
        fprintf(stderr, "This will run approximated subtraction tests on random test cases with 512 bits\n");
        fprintf(stderr, "Note: The test cases are stored in the ./cases directory\n");
        fprintf(stderr, "The test cases are in csv.gzip format\n");
        return EXIT_FAILURE;
    }

    int op = atoi(argv[1]);
    int NUM_BITS = atoi(argv[2]);
    int case_type = atoi(argv[3]);

    assert(op >= 0 && op <= 3);
    assert(NUM_BITS > 0 && NUM_BITS <= 131072);
    assert(case_type == 0 || case_type == 1);

    run_test(op, NUM_BITS, case_type);

    return EXIT_SUCCESS;
}