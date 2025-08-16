#!/bin/bash
#!/bin/bash

# First Compile test.c
gcc test.c ../../utils/perf_utils.c -o test -ldot -lz -I../../utils/
bit_sizes=(256 512 1024 2048 4096 8192 16384 32768 65536 131072)
operations=(0 1 2 3)
# operation name: 0 for dot_add, 1 for dot_sub
operation_name=(dot_add dot_sub dot_add_approx dot_sub_approx)

echo "Running user instruction count and tick count tests"
echo "----------------------------------------"

# Loop through each operations
for operation in "${operations[@]}"; do
    # Loop through each bit size
    echo "Running tests for operation ${operation_name[$operation]}"
    echo "----------------------------------------"
    for bit_size in "${bit_sizes[@]}"; do
        echo "Running test for bit size $bit_size"
        # Run the test with the current operation and bit size
        taskset -c 0 ./test $operation $bit_size 1
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