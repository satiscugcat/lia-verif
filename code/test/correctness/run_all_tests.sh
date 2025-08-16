#!/bin/bash
# filepath: /home/subhrajit/DigitsOnTurbo/code/test/correctness/run_all_tests.sh

# First Compile test.c
gcc test.c -o test -ldot -lz -O2

bit_sizes=(256 260 512 520 1024 1036 2048 2052 4096 4104 8192 8204 16384 16388 32768 32776)
operations=(0 1)
# operation name: 0 for dot_add, 1 for dot_sub, 2 for dot_add_approx, 3 for dot_sub_approx
operation_name=(dot_add dot_sub)
# case types: 0 for random, 1 for special
case_types=(1)
case_name=(special)

# Loop through each operation
for operation in "${operations[@]}"; do
    echo "===== Running tests for operation ${operation_name[$operation]} ====="
    
    # Determine which case types to use based on the operation
    # Skip special test cases for approximated operations (2 and 3)
    current_case_types=("${case_types[@]}")
    if [ $operation -eq 2 ] || [ $operation -eq 3 ]; then
        # Only use random test cases for approximated operations
        current_case_types=(0)
        echo "Note: Skipping special test cases for approximated operation"
    fi
    
    # Loop through applicable case types
    for case_type in "${current_case_types[@]}"; do
        echo "--- Testing with ${case_name[$case_type]} test cases ---"
        
        # Loop through each bit size
        for bit_size in "${bit_sizes[@]}"; do
            echo "Testing ${operation_name[$operation]} with ${bit_size}-bit ${case_name[$case_type]} test cases..."
            
            # Run the test with the current operation, bit size, and case type
            ./test $operation $bit_size $case_type
            
            # Check if the test was successful
            if [ $? -ne 0 ]; then
                echo "❌ Test failed for operation ${operation_name[$operation]}, bit size $bit_size, case type ${case_name[$case_type]}"
                exit 1
            else
                echo "✅ Test passed for operation ${operation_name[$operation]}, bit size $bit_size, case type ${case_name[$case_type]}"
            fi
        done
    done
    
    echo " Hurrah! All tests for operation ${operation_name[$operation]} passed!"
    echo "----------------------------------------"
    echo "----------------------------------------"
    echo "----------------------------------------"
    echo ""
done

echo "🎉 All correctness tests completed successfully!"