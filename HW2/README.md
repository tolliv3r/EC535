# EC535 HW2 - Instruction Set Simulator

## Overview
This is an instruction set simulator written in C that simulates the execution of assembly code and tracks performance metrics including instruction count, clock cycles, memory hits, and load/store operations.

## Features
- Parses assembly code from input files
- Simulates a simple cache with 2-cycle hit time and 10-cycle miss penalty
- Tracks execution statistics:
  - Total executed instructions
  - Total clock cycles
  - Number of cache hits
  - Total LD/ST instructions

## Files
- `myISS.c` - Main simulator implementation
- `Makefile` - Build configuration
- `sample.assembly` - Sample assembly code for testing
- `test_cache.assembly` - Cache behavior test

## Building and Running

### Build the simulator:
```bash
make
```

### Run the simulator:
```bash
./myISS <assembly_file>
```

### Clean build files:
```bash
make clean
```

## Example Usage
```bash
$ ./myISS sample.assembly
Total number of executed instructions: 15
Total number of clock cycles: 101
Number of hits to local memory: 3
Total number of executed LD/ST instructions: 11
```

## Implementation Details

### Cache Simulation
- Simple cache that stores accessed memory addresses
- First access to any address is a miss (10 cycles)
- Subsequent accesses to the same address are hits (2 cycles)
- Cache has unlimited capacity for simplicity

### Instruction Parsing
- Supports basic assembly instructions
- Recognizes LD (load) and ST (store) instructions
- Extracts memory addresses from bracket notation (e.g., `[0x1000]`)
- Skips comments, empty lines, and labels

### Supported Instructions
The simulator recognizes and processes:
- LD (Load) instructions
- ST (Store) instructions
- Other instructions (ADD, SUB, MUL, etc.) are counted but don't affect memory simulation

## Testing
Two test files are provided:
1. `sample.assembly` - General functionality test
2. `test_cache.assembly` - Specific cache behavior verification

The cache test verifies:
- First access to 0x1000: miss (10 cycles)
- Second access to 0x1000: hit (2 cycles)
- Third access to 0x1000: hit (2 cycles)
- Access to 0x2000: miss (10 cycles)
- Fourth access to 0x1000: hit (2 cycles)
- Store to 0x3000: miss (10 cycles)
- Store to 0x1000: hit (2 cycles)

Total: 7 instructions, 4 hits, 3 misses = 7 + (3×10) + (4×2) = 45 cycles ✓
