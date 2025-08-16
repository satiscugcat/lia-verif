`test.c` tests the correctness of the computations.

First generate the test cases:
```bash
~/DigitsOnTurbo/code/test/correctness$ python gen_all_cases.py
```

First make sure that below dependencies are installed:

### For Ubuntu/Debian:
```bash
sudo apt update -y
sudo apt upgrade -y
sudo apt-get install -y libgmp3-dev
sudo apt-get install -y linux-tools-common linux-tools-generic linux-tools-`uname -r`
sudo apt install -y python3-pip
pip3 install gmpy2
```

### For Fedora:
```bash
sudo dnf update -y
sudo dnf upgrade -y
sudo dnf install -y gmp-devel
sudo dnf install -y perf
sudo dnf install -y python3-pip
pip3 install gmpy2
```

Then compile and run individual test cases:
```bash
gcc test.c ../../src/dot_add.c ../../src/dot_sub.c ../../src/utils/dot_utils.c -o test -I../../src/ -I../../src/utils/ -lz -O1 -mavx512f -mavx512vl
./test <operation> <bit size>
```

Where `<operation>` is one of the following:
- 0: add
- 1: sub

And `<bit size>` is one of the following:
- 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072

If you want to compile and run all the test cases, you can use the following command:
```bash
./run_all_tests.sh
```
