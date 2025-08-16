#!/usr/bin/env python3
# filepath: /home/subhrajit/DigitsOnTurbo/code/test/correctness/gen_all_cases.py

import os
import time
import multiprocessing
import subprocess
import sys
from tqdm import tqdm

# Define operations and bit sizes
operations = ["add", "sub"]
bit_sizes = [256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072]
# bit_sizes = [260, 520, 1036, 2052, 4104, 8204, 16388, 32776]

def format_time(seconds):
    """Format seconds into hours:minutes:seconds."""
    hours, remainder = divmod(seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{int(hours):02d}:{int(minutes):02d}:{int(seconds):02d}"

def generate_testcase(args):
    """Generate test cases for a specific operation and bit size."""
    op, size = args
    
    # Create necessary directories
    os.makedirs(f"cases/{op}/{size}", exist_ok=True)
    
    # Call the appropriate Python script based on operation
    if op == "mul":
        script_name = "__gen_mul.py"
    elif op == "sqr":
        script_name = "__gen_sqr.py"
    elif op == "gcd":
        script_name = "__gen_gcd.py"
    else:
        script_name = "__gen_cases.py"
    
    result = subprocess.run(
        ["python3", script_name, op, str(size)],
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        return (False, f"Error processing {op} with {size} bits: {result.stderr}")
    
    return (True, f"Completed {op} with {size} bits")

def main():
    # Create necessary directories
    for op in operations:
        os.makedirs(f"cases/{op}", exist_ok=True)

    # Generate all combinations
    tasks = [(op, size) for op in operations for size in bit_sizes]
    total_tasks = len(tasks)

    print("Starting parallel test case generation...")
    print("=========================================")
    
    # Track total execution time
    start_time = time.time()
    
    # Create a pool of workers - use all available cores
    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        # Use tqdm to track progress
        results = []
        for result in tqdm(
            pool.imap_unordered(generate_testcase, tasks),
            total=total_tasks,
            desc="Generating test cases"
        ):
            results.append(result[0])  # Store success/failure
            # Don't print during progress bar updates
    
    # Now print completion messages
    print("\nTask details:")
    for result in results:
        if isinstance(result, tuple) and len(result) > 1:
            print(result[1])  # Print stored message
    
    # Calculate execution time
    end_time = time.time()
    total_time = end_time - start_time
    
    print("\n=========================================")
    print("All test cases generated!")
    print(f"Total execution time: {format_time(total_time)}")
    print("=========================================")
    
    # Check if all tasks completed successfully
    success_count = sum(1 for r in results if r is True)
    if success_count == total_tasks:
        print("All test cases were generated successfully.")
    else:
        print(f"Warning: {total_tasks - success_count} out of {total_tasks} test cases failed to generate.")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())