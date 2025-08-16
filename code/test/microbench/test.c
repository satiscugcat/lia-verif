#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdbool.h>
#include <assert.h>
#include <cpuid.h>
#include <errno.h>
#include <ctype.h>
#include "dotlib.h"
#include "perf_utils.h"
#include "timing_utils.h"

#define RANDOM_ITERATIONS 100000 // Number of random test cases
#define SPECIAL_ITERATIONS 1000  // Number of special test cases
#define CHUNK 655360             // Chunk size for reading the file
int CORE_NO = 0;                 // CPU core number for performance measurement

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

void run_perf_test(int op, int NUM_BITS, int test_case_type)
{
    char test_filename[100];
    const char *file_type = (test_case_type == 0) ? "random" : "special";
    int iterations = (test_case_type == 0) ? RANDOM_ITERATIONS : SPECIAL_ITERATIONS;

    snprintf(test_filename, sizeof(test_filename), "../correctness/cases/%s/%d/%s.csv.gz",
             op % 2 ? "sub" : "add", NUM_BITS, file_type);

    printf("Running performance test on %s test cases for %s with %d bits\n",
           file_type, op <= 1 ? (op == 0 ? "addition" : "subtraction") : (op == 2 ? "approximated addition" : "approximated subtraction"), NUM_BITS);

    gzFile test_file = open_gzfile(test_filename, "rb");
    srand(time(NULL));
    int i = rand() % iterations;
    char buffer[CHUNK];
    gzrewind(test_file);
    skip_first_line(test_file);

    // read ith line from the test_file
    for (int j = 0; j < i; j++)
    {
        // flush the buffer
        memset(buffer, 0, CHUNK);
        if (gzgets(test_file, buffer, sizeof(buffer)) == NULL)
        {
            if (gzeof(test_file))
            {
                fprintf(stderr, "Reached end of file before finding test case %d\n", i);
                gzclose(test_file);
                exit(EXIT_FAILURE);
            }
            else
            {
                perror("Error reading line");
                gzclose(test_file);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Read the actual line we want to test
    if (gzgets(test_file, buffer, sizeof(buffer)) == NULL)
    {
        fprintf(stderr, "Failed to read test case %d\n", i);
        gzclose(test_file);
        exit(EXIT_FAILURE);
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

    // printf("Testing with case #%d:\n", i);
    // printf("A: %s\n", a_str);
    // printf("B: %s\n", b_str);

    init_memory_pool();
    dot_limb_t *a, *b;
    a = dot_limb_set_str(a_str);
    b = dot_limb_set_str(b_str);

    // adjust the sizes of a and b
    dot_limb_t_adjust_sizes(a, b);
    typedef void (*dot_operation_func)(dot_limb_t *, dot_limb_t *, dot_limb_t *);
    dot_operation_func func = op == 0 ? dot_add_n : (op == 1 ? dot_sub_n : (op == 2 ? dot_add_n_approx : dot_sub_n_approx));

    int n = a->size;
    dot_limb_t *s = dot_limb_t_alloc(n);
    if (s == NULL)
    {
        perror("Memory allocation failed for s\n");
        exit(EXIT_FAILURE);
    }

    func(s, a, b);

    long long values_overhead[MAX_EVENTS];
    long long values[MAX_EVENTS];

    initialize_perf();

    // perf overhead
    start_perf();

    stop_perf();

    read_perf(values_overhead);

    // clear cache content for a_limbs, b_limbs
    for (int i = 0; i < n; i += 64)
    {
        _mm_clflush((char *)a + i);
        _mm_clflush((char *)b + i);
    }

    _mm_mfence();

    start_perf();

    func(s, a, b);

    stop_perf();

    read_perf(values);

    printf("User instructions: %lld\n", values[0] - values_overhead[0]);

    func(s, a, b);

    double t;
    RECORD_RDTSC(t, func(s, a, b));

    printf("Avg. RDTSC Ticks: %f\n", t);

    destroy_memory_pool();

    gzclose(test_file);
}

void run_timing_test(int op, int NUM_BITS, int test_case_type)
{
    char test_filename[100];
    const char *file_type = (test_case_type == 0) ? "random" : "special";
    int iterations = (test_case_type == 0) ? RANDOM_ITERATIONS : SPECIAL_ITERATIONS;

    snprintf(test_filename, sizeof(test_filename), "../correctness/cases/%s/%d/%s.csv.gz",
             op % 2 ? "sub" : "add", NUM_BITS, file_type);

    printf("Running timing test on %s test cases for %s with %d bits\n",
           file_type, op <= 1 ? (op == 0 ? "addition" : "subtraction") : (op == 2 ? "approximated addition" : "approximated subtraction"), NUM_BITS);

    gzFile test_file = open_gzfile(test_filename, "rb");
    srand(time(NULL));
    int i = rand() % iterations;
    char buffer[CHUNK];
    gzrewind(test_file);
    skip_first_line(test_file);

    // read ith line from the test_file
    for (int j = 0; j < i; j++)
    {
        // flush the buffer
        memset(buffer, 0, CHUNK);
        if (gzgets(test_file, buffer, sizeof(buffer)) == NULL)
        {
            if (gzeof(test_file))
            {
                fprintf(stderr, "Reached end of file before finding test case %d\n", i);
                gzclose(test_file);
                exit(EXIT_FAILURE);
            }
            else
            {
                perror("Error reading line");
                gzclose(test_file);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Read the actual line we want to test
    if (gzgets(test_file, buffer, sizeof(buffer)) == NULL)
    {
        fprintf(stderr, "Failed to read test case %d\n", i);
        gzclose(test_file);
        exit(EXIT_FAILURE);
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

    // printf("Testing with case #%d:\n", i);
    // printf("A: %s\n", a_str);
    // printf("B: %s\n", b_str);

    init_memory_pool();
    dot_limb_t *a, *b;
    a = dot_limb_set_str(a_str);
    b = dot_limb_set_str(b_str);

    // adjust the sizes of a and b
    dot_limb_t_adjust_sizes(a, b);
    typedef void (*dot_operation_func)(dot_limb_t *, dot_limb_t *, dot_limb_t *);
    dot_operation_func func = op == 0 ? dot_add_n : (op == 1 ? dot_sub_n : (op == 2 ? dot_add_n_approx : dot_sub_n_approx));

    int n = a->size;
    dot_limb_t *s = dot_limb_t_alloc(n);
    if (s == NULL)
    {
        perror("Memory allocation failed for s\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i += 64)
    {
        _mm_clflush((char *)a + i);
        _mm_clflush((char *)b + i);
    }

    double time_taken = 0;
    unsigned long long int t0, t1, niter;
    double f, ops_per_sec;
    int decimals = 0;
    int cpu_info[4];

    fflush(stdout);

    __cpuid(0, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);

    TIME_RUSAGE(time_taken, func(s, a, b));

    printf("Execution time: %.1f ns\n", time_taken * 1000);

    niter = 1 + (unsigned long)(1e7 / time_taken);

    fflush(stdout);

    t0 = cputime();
    for (int i = 0; i < niter; i++)
    {
        func(s, a, b);
    }
    t1 = cputime() - t0;

    ops_per_sec = (1e6 * niter) / t1;
    f = 100.0;

    for (decimals = 0;; decimals++)
    {
        if (ops_per_sec > f)
            break;
        f = f * 0.1;
    }

    printf("Throughput: %.*f OP/s\n", decimals, ops_per_sec);

    destroy_memory_pool();

    gzclose(test_file);
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <operation> <number_of_bits> <test_type> <case_type>\n", argv[0]);
        fprintf(stderr, "operation: 0 for addition, 1 for subtraction, 2 for approximated addition, 3 for approximated subtraction\n");
        fprintf(stderr, "number_of_bits: number of bits for the test case\n");
        fprintf(stderr, "test_type: 0 for timing and throughput, 1 for user instructions and ticks\n");
        fprintf(stderr, "case_type: 0 for random test cases, 1 for special test cases\n");
        return EXIT_FAILURE;
    }

    int op = atoi(argv[1]);
    int NUM_BITS = atoi(argv[2]);
    int test_type = atoi(argv[3]);
    int case_type = atoi(argv[4]);

    assert(op >= 0 && op <= 3);
    assert(test_type == 0 || test_type == 1);
    assert(case_type == 0 || case_type == 1);
    assert(NUM_BITS > 0 && NUM_BITS <= 131072);

    if (test_type == 0)
    {
        // Run the timing and throughput test
        run_timing_test(op, NUM_BITS, case_type);
    }
    else if (test_type == 1)
    {
        // Run the user instructions and ticks test
        run_perf_test(op, NUM_BITS, case_type);
    }
    else
    {
        fprintf(stderr, "Invalid test type: %d\n", test_type);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}