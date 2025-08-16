#!/bin/bash

# First Compile test.c
gcc test.c ../../utils/perf_utils.c -o test -ldot -lz -I../../utils/ -O1
bit_sizes=(256 260 512 520 1024 1036 2048 2052 4096 4104 8192 8204 16384 16388 32768 32776)
operations=(0)
# operation name: 0 for dot_add, 1 for dot_sub
operation_name=(dot_add dot_sub dot_add_approx dot_sub_approx)

echo "Running timing and throughput tests"
echo "----------------------------------------"

# Loop through each operations
for operation in "${operations[@]}"; do
    # Loop through each bit size
    echo "Running tests for operation ${operation_name[$operation]}"
    echo "----------------------------------------"
    for bit_size in "${bit_sizes[@]}"; do
        echo "Running test for bit size $bit_size"
        # Run the test with the current operation and bit size
        taskset -c 0 ./test $operation $bit_size 0 0
        # Check if the test was successful
        if [ $? -ne 0 ]; then
            echo "Test failed for operation ${operation_name[$operation]} and bit size $bit_size"
            exit 1
        fi
        # newline
        echo ""
    done
    echo "All tests done for operation ${operation_name[$operation]}"
    echo "----------------------------------------"
done