import gmpy2
import random
import os
from time import time
import gzip

BUFFER_SIZE = 1000
LIMB_SIZE = 64  # 64-bit limbs

def generate_seed():
    return random.SystemRandom().randint(1, 2**32 - 1)

def generate_random_testcases(filename, bit_size, num_testcases, operation):
    random.seed(generate_seed())
    state = gmpy2.random_state(random.getrandbits(32))

    buffer = []
    with gzip.open(filename, 'wt') as f:
        f.write("num1,num2,result\n")
        for _ in range(num_testcases):
            try:
                a = gmpy2.mpz_urandomb(state, bit_size)
                b = gmpy2.mpz_urandomb(state, bit_size)
                c = a + b if operation == "add" else a - b
                buffer.append(f"{a:x},{b:x},{c:x}\n")

                if len(buffer) >= BUFFER_SIZE:
                    f.writelines(buffer)
                    buffer = []
            except Exception as e:
                print(f"Error in random test case generation: {e}")

        if buffer:
            f.writelines(buffer)

def generate_special_testcases(filename, bit_size, num_special_cases, operation):
    seed = generate_seed()
    random.seed(seed)
    state = gmpy2.random_state(random.getrandbits(32))
    max_limb = gmpy2.mpz(2**LIMB_SIZE - 1)
    num_limbs = bit_size // LIMB_SIZE

    # if bit_size % LIMB_SIZE != 0:
    #     raise ValueError("Bit size must be divisible by limb size (64 bits)")

    # Calculate cases per category to get exact count
    cases_per_category = num_special_cases // 5  # For 5 categories
    extra_cases = num_special_cases % 5  # Distribute extras
    category_counts = [cases_per_category] * 5
    for i in range(extra_cases):
        category_counts[i] += 1

    testcases = []

    try:
        # 1. Full propagation chain: A = 0xFFFF...FF, B = 1 and variations
        count = 0
        for i in range(category_counts[0]):
            try:
                a = gmpy2.mpz(0)
                for _ in range(num_limbs):
                    a = (a << LIMB_SIZE) | max_limb
                if i == 0:  # Explicitly include A=0xFFFF...FF, B=1 for add, or A=B for sub
                    if operation == "add":
                        b = gmpy2.mpz(1)
                        comment = "Full carry chain: A=0xFFFF...FF, B=1, R=0x10000...00"
                    else:  # Subtraction: A = B for full borrow chain
                        b = a
                        comment = "Full borrow chain: A=0xFFFF...FF, B=0xFFFF...FF, R=0"
                elif i < min(100, category_counts[0]):  # Small B values
                    b = gmpy2.mpz(i + 2)
                    comment = f"Full {'carry' if operation == 'add' else 'borrow'} chain: A=0xFFFF...FF, B={b}"
                else:  # Random large B
                    random.seed(seed + i)
                    state = gmpy2.random_state(random.getrandbits(32))
                    b = gmpy2.mpz_urandomb(state, bit_size // 2) | (1 << (bit_size // 2 - 1))
                    comment = f"Full {'carry' if operation == 'add' else 'borrow'} chain: A=0xFFFF...FF, B=large"

                c = a + b if operation == "add" else a - b
                testcases.append(f"{a:x},{b:x},{c:x},{comment}\n")
                count += 1
            except Exception as e:
                print(f"Error in full propagation case {i}: {e}")
        print(f"Generated {count} full propagation test cases")

        # 2. Maxed-out/zero limbs: Each limb sum = 2^64 - 1 (add) or zero (sub)
        count = 0
        for i in range(category_counts[1]):
            try:
                random.seed(seed + i + category_counts[0])
                state = gmpy2.random_state(random.getrandbits(32))
                a = gmpy2.mpz(0)
                b = gmpy2.mpz(0)
                for _ in range(num_limbs):
                    if operation == "sub" and i < category_counts[1] // 2:  # A_i = B_i for zero limbs
                        k = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                        a_limb = k
                        b_limb = k
                    else:  # A_i + B_i = 2^64 - 1 for add
                        k = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                        a_limb = k
                        b_limb = max_limb - k
                    a = (a << LIMB_SIZE) | a_limb
                    b = (b << LIMB_SIZE) | b_limb

                if operation == "add":
                    c = a + b
                    comment = "Maxed-out limbs: Each limb sum = 2^64 - 1"
                else:
                    c = a - b
                    comment = "Zero limbs: Each limb A_i = B_i, result = 0"

                testcases.append(f"{a:x},{b:x},{c:x},{comment}\n")
                count += 1
            except Exception as e:
                print(f"Error in maxed-out/zero limbs case {i}: {e}")
        print(f"Generated {count} maxed-out/zero limbs test cases")

        # 3. Carry-heavy cases: Frequent carries
        count = 0
        for i in range(category_counts[2]):
            try:
                random.seed(seed + i + sum(category_counts[:2]))
                state = gmpy2.random_state(random.getrandbits(32))
                a = gmpy2.mpz(0)
                b = gmpy2.mpz(0)
                for j in range(num_limbs):
                    a_limb = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                    b_limb = max_limb - gmpy2.mpz_urandomb(state, LIMB_SIZE // 2) if j % 2 == 0 else gmpy2.mpz_urandomb(state, LIMB_SIZE) | (1 << (LIMB_SIZE - 1))
                    a = (a << LIMB_SIZE) | a_limb
                    b = (b << LIMB_SIZE) | b_limb

                c = a + b if operation == "add" else a - b
                comment = "Carry-heavy: Frequent carries with alternating pattern" if operation == "add" else "Carry-heavy: Random subtraction"
                testcases.append(f"{a:x},{b:x},{c:x},{comment}\n")
                count += 1
            except Exception as e:
                print(f"Error in carry-heavy case {i}: {e}")
        print(f"Generated {count} carry-heavy test cases")

        # 4. Borrow-heavy cases: Frequent borrows for subtraction, ensuring A > B
        count = 0
        for i in range(category_counts[3]):
            try:
                random.seed(seed + i + sum(category_counts[:3]))
                state = gmpy2.random_state(random.getrandbits(32))
                b = gmpy2.mpz_urandomb(state, bit_size)
                # Ensure A > B by adding a small positive value
                small_value = gmpy2.mpz_urandomb(state, 16) + 1  # Small value in [1, 2^16]
                a = b + small_value
                # Adjust some limbs to trigger borrows
                a_limbs = []
                b_limbs = []
                for j in range(num_limbs):
                    b_limb = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                    # 50% chance to make A_i <= B_i for borrow
                    if random.random() < 0.5:
                        a_limb = b_limb - gmpy2.mpz_urandomb(state, 8)  # A_i <= B_i
                        if a_limb < 0:
                            a_limb = b_limb
                    else:
                        a_limb = b_limb + gmpy2.mpz_urandomb(state, LIMB_SIZE // 2)
                    a_limbs.append(a_limb)
                    b_limbs.append(b_limb)
                # Reconstruct A and B to ensure A > B
                a = gmpy2.mpz(0)
                b = gmpy2.mpz(0)
                for j in range(num_limbs):
                    a = (a << LIMB_SIZE) | a_limbs[j]
                    b = (b << LIMB_SIZE) | b_limbs[j]
                # Final check: if A <= B, adjust A
                if a <= b:
                    a = b + gmpy2.mpz_urandomb(state, 16) + 1

                comment = "Borrow-heavy: Frequent borrows" if operation == "sub" else "Borrow-heavy: Random addition"
                c = a + b if operation == "add" else a - b
                testcases.append(f"{a:x},{b:x},{c:x},{comment}\n")
                count += 1
            except Exception as e:
                print(f"Error in borrow-heavy case {i}: {e}")
        print(f"Generated {count} borrow-heavy test cases")

        # 5. Edge and mixed cases
        count = 0
        for i in range(category_counts[4]):
            try:
                random.seed(seed + i + sum(category_counts[:4]))
                state = gmpy2.random_state(random.getrandbits(32))
                if i < category_counts[4] // 4:  # A = 0, B = max
                    a = gmpy2.mpz(0)
                    b = (gmpy2.mpz(1) << bit_size) - 1
                    comment = "Edge case: A=0, B=max"
                elif i < category_counts[4] // 2:  # A = max, B = 0
                    a = (gmpy2.mpz(1) << bit_size) - 1
                    b = gmpy2.mpz(0)
                    comment = "Edge case: A=max, B=0"
                elif i < 3 * category_counts[4] // 4:  # A = max, B = small
                    a = (gmpy2.mpz(1) << bit_size) - 1
                    b = gmpy2.mpz_urandomb(state, 32)
                    comment = "Edge case: A=max, B=small"
                else:  # Mixed propagation
                    a = gmpy2.mpz(0)
                    b = gmpy2.mpz(0)
                    for j in range(num_limbs):
                        if j % 2 == 0:
                            k = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                            a_limb = k
                            b_limb = max_limb - k
                        else:
                            a_limb = gmpy2.mpz_urandomb(state, LIMB_SIZE)
                            b_limb = max_limb - gmpy2.mpz_urandomb(state, LIMB_SIZE // 2)
                        a = (a << LIMB_SIZE) | a_limb
                        b = (b << LIMB_SIZE) | b_limb
                    comment = "Mixed propagation: Alternating maxed and carry-prone limbs"

                c = a + b if operation == "add" else a - b
                testcases.append(f"{a:x},{b:x},{c:x},{comment}\n")
                count += 1
            except Exception as e:
                print(f"Error in edge/mixed case {i}: {e}")
        print(f"Generated {count} edge and mixed test cases")

        # Write all test cases to file
        with gzip.open(filename, 'wt') as f:
            f.write("num1,num2,result,comment\n")
            f.writelines(testcases)

    except Exception as e:
        print(f"Error writing special test cases: {e}")

    # Verify exactly the expected number of test cases
    actual_count = len(testcases)
    if actual_count != num_special_cases:
        print(f"Warning: Generated {actual_count} special test cases instead of {num_special_cases}")
    else:
        print(f"Successfully generated exactly {num_special_cases} special test cases")

def main(operation, bit_size, num_testcases, num_special_cases):
    os.makedirs(f"cases/{operation}/{bit_size}", exist_ok=True)
    
    try:
        # Generate random test cases
        generate_random_testcases(f"cases/{operation}/{bit_size}/random.csv.gz", bit_size, num_testcases, operation)
        # Generate special test cases
        generate_special_testcases(f"cases/{operation}/{bit_size}/special.csv.gz", bit_size, num_special_cases, operation)
    except Exception as e:
        print(f"Error in main: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Usage: python gen_testcases.py <operation> <bit-size>")
        sys.exit(1)

    operation = sys.argv[1]
    bit_size = int(sys.argv[2])
    if operation not in ['add', 'sub']:
        print("Operation must be 'add' or 'sub'")
        sys.exit(1)

    if bit_size < 256 or bit_size > 131072:
        print("Bit size must be between 256 and 131072")
        sys.exit(1)

    # if bit_size % LIMB_SIZE != 0:
    #     print(f"Bit size must be divisible by {LIMB_SIZE}")
    #     sys.exit(1)
    
    num_testcases = 100000
    num_special_cases = 1000
    start_time = time()
    print(f"Generating {num_testcases} random and {num_special_cases} special test cases of size {bit_size} for operation {operation}...")
    main(operation, bit_size, num_testcases, num_special_cases)
    end_time = time()
    print(f"Test cases generated in {end_time - start_time:.2f} seconds.")
    print(f"Random test cases saved to cases/{operation}/{bit_size}/random.csv.gz")
    print(f"Special test cases saved to cases/{operation}/{bit_size}/special.csv.gz")