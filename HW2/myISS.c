#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 256
#define MAX_MEMORY_ADDRESSES 10000
#define CACHE_HIT_CYCLES 2
#define CACHE_MISS_CYCLES 10  // Assuming miss penalty is 10 cycles (can be adjusted)

// Global statistics
typedef struct {
    uint32_t total_instructions;
    uint32_t total_cycles;
    uint32_t memory_hits;
    uint32_t ld_st_instructions;
} SimulatorStats;

// Simple cache simulation
typedef struct {
    uint32_t addresses[MAX_MEMORY_ADDRESSES];
    int count;
} Cache;

// Global variables
SimulatorStats stats = {0, 0, 0, 0};
Cache cache = {0};

// Function to check if an address is in cache
int is_cache_hit(uint32_t address) {
    for (int i = 0; i < cache.count; i++) {
        if (cache.addresses[i] == address) {
            return 1;
        }
    }
    return 0;
}

// Function to add address to cache
void add_to_cache(uint32_t address) {
    if (cache.count < MAX_MEMORY_ADDRESSES) {
        cache.addresses[cache.count] = address;
        cache.count++;
    }
}

// Function to simulate memory access
uint32_t simulate_memory_access(uint32_t address) {
    if (is_cache_hit(address)) {
        stats.memory_hits++;
        return CACHE_HIT_CYCLES;
    } else {
        add_to_cache(address);
        return CACHE_MISS_CYCLES;
    }
}

// Function to parse and execute an instruction
void execute_instruction(char* line) {
    char instruction[32];
    char operand1[32], operand2[32], operand3[32];
    uint32_t cycles = 1; // Base cycle count for any instruction
    
    // Parse the instruction line
    int parsed = sscanf(line, "%s %s %s %s", instruction, operand1, operand2, operand3);
    
    if (parsed < 1) return;
    
    stats.total_instructions++;
    
    // Handle different instruction types
    if (strcmp(instruction, "LD") == 0 || strcmp(instruction, "ST") == 0) {
        stats.ld_st_instructions++;
        
        // Extract memory address from operand (assuming format like "R1, [R2]" or "R1, [0x1000]")
        uint32_t mem_address = 0;
        
        // Simple address extraction - look for hex or decimal numbers in brackets
        char* bracket_start = strchr(line, '[');
        if (bracket_start) {
            char* bracket_end = strchr(bracket_start, ']');
            if (bracket_end) {
                char address_str[32];
                int addr_len = bracket_end - bracket_start - 1;
                if (addr_len > 0 && addr_len < 32) {
                    strncpy(address_str, bracket_start + 1, addr_len);
                    address_str[addr_len] = '\0';
                    
                    // Parse hex or decimal address
                    if (strstr(address_str, "0x") == address_str) {
                        mem_address = strtoul(address_str, NULL, 16);
                    } else {
                        mem_address = strtoul(address_str, NULL, 10);
                    }
                }
            }
        }
        
        // Add memory access cycles
        cycles += simulate_memory_access(mem_address);
    }
    
    stats.total_cycles += cycles;
}

// Function to read and process assembly file
void process_assembly_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(1);
    }
    
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Skip labels (lines ending with ':')
        if (line[strlen(line) - 1] == ':') {
            continue;
        }
        
        execute_instruction(line);
    }
    
    fclose(file);
}

// Function to print results
void print_results() {
    printf("Total number of executed instructions: %u\n", stats.total_instructions);
    printf("Total number of clock cycles: %u\n", stats.total_cycles);
    printf("Number of hits to local memory: %u\n", stats.memory_hits);
    printf("Total number of executed LD/ST instructions: %u\n", stats.ld_st_instructions);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <assembly_file>\n", argv[0]);
        exit(1);
    }
    
    process_assembly_file(argv[1]);
    print_results();
    
    return 0;
}
