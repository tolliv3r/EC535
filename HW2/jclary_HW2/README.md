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

#### Compilation Optimizations
- Uses `-std=c99` for consistent, optimized behavior across platforms
- `-Wall -Wextra` ensures code quality without performance impact

#### Data Structure Optimizations
- Uses pre-allocated arrays instead of dynamic allocation
- Uses `uint32_t` for counters and addresses

#### Algorithm Optimizations
- Simple O(n) search
- Skips processing for comments, empty lines, and labels
- Processes each line only once

#### Memory Access Optimizations
- Uses local arrays instead of heap allocation
- Uses `strchr()` and `strncpy()` for fast substring extraction
- `strcspn()` for efficient newline removal

#### I/O Optimizations
- Uses `fgets()` with fixed buffer size