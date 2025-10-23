# EC535 HW2

## Overview
This is an instruction set simulator written in C that simulates the execution of assembly code and tracks performance metrics including instruction count, clock cycles, memory hits, and load/store operations.

## Files
- `myISS.c`
- `Makefile`
- `sample.assembly`
- `test_cache.assembly`

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

## Testing
1. `sample.assembly`
2. `test_cache.assembly`

### Performance Optimizations

#### Data Structure Optimizations
- Used pre-allocated arrays instead of dynamic allocation
- Used `uint32_t` for counters and addresses

#### Algorithm Optimizations
- Simple O(n) search with early exit for faster average lookups
- Add valid bit tracking in cache simulation for safer and more predictable behavior  
    - linear search, true hash table maybe doable
- Skip processing for comments, empty lines, and labels
- Process each line only once

#### Memory Access Optimizations
- Local arrays instead of heap allocation
- Used `strchr()` and `strncpy()` for fast substring extraction
- `strcspn()` for efficient newline removal

#### I/O Optimizations
- Used `fgets()` with fixed buffer size
