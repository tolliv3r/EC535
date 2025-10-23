#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 256
#define MAX_MEMORY_ADDRESSES 10000
#define CACHE_HIT_CYCLES 2
#define CACHE_MISS_CYCLES 50 // miss penalty
#define LOCAL_MEMORY_SIZE 256 // 256-byte local memory

// global structs
typedef struct {
    uint32_t total_instructions;
    uint32_t total_cycles;
    uint32_t memory_hits;
    uint32_t ld_st_instructions;
} SimulatorStats;

// optimized with validity tracking
typedef struct {
    uint32_t addresses[MAX_MEMORY_ADDRESSES];
    uint8_t valid[MAX_MEMORY_ADDRESSES];  // track which slots are valid
    int count;
} Cache;

// cpu simulation
typedef struct {
    int8_t registers[7]; // R1-R6 (index 1-6), index 0 unused
    uint32_t pc; // program counter
    uint8_t zero_flag; // for cmp instruction
} CPU;

// global variables
SimulatorStats stats = {0, 0, 0, 0};
Cache cache = {0};
CPU cpu = {0};

// global arrays for line number mapping
char global_lines[1000][MAX_LINE_LENGTH];
int global_line_numbers[1000];
int global_line_count = 0;

// find instruction index from line number
int find_instruction_index(int target_line) {
    if (global_line_count > 0 && global_line_numbers[0] > 0) {
        // has explicit line numbers
        for (int i = 0; i < global_line_count; i++) {
            if (global_line_numbers[i] == target_line) {
                return i;
            }
        }
        return -1; // line not found
    } else {
        // no explicit line numbers
        if (target_line >= 1 && target_line <= global_line_count) {
            return target_line - 1; // convert to 0-based index
        }
        return -1; // invalid index
    }
}

// optimized cache hit check, exits early to skip the rest of the loop
int is_cache_hit(uint32_t address) {
    for (int i = 0; i < cache.count; i++) {
        if (cache.valid[i] && cache.addresses[i] == address) {
            return 1;
        }
    }
    return 0;
}

// optimized cache add
void add_to_cache(uint32_t address) {
    if (cache.count < MAX_MEMORY_ADDRESSES) {
        cache.addresses[cache.count] = address;
        cache.valid[cache.count] = 1;
        cache.count++;
    }
}

// optimized memory access simulation
uint32_t simulate_memory_access(uint32_t address) {
    // validate address is within 256-byte local memory range
    // wrap around
    uint8_t wrapped_address = (uint8_t)address;
    
    if (is_cache_hit(wrapped_address)) {
        stats.memory_hits++;
        return CACHE_HIT_CYCLES;
    } else {
        add_to_cache(wrapped_address);
        return CACHE_MISS_CYCLES;
    }
}

// fast integer parsing without strtoul
uint32_t fast_parse_int(const char* str, int len) {
    uint32_t result = 0;
    int i = 0;
    int negative = 0;
    
    // handle negative numbers
    if (len > 0 && str[0] == '-') {
        negative = 1;
        i = 1;
    }
    
    // handle hex prefix
    if (len > i + 2 && str[i] == '0' && str[i+1] == 'x') {
        i += 2;
        for (; i < len; i++) {
            char c = str[i];
            if (c >= '0' && c <= '9') {
                result = result * 16 + (c - '0');
            } else if (c >= 'a' && c <= 'f') {
                result = result * 16 + (c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                result = result * 16 + (c - 'A' + 10);
            }
        }
    } else {
        // decimal parsing
        for (; i < len; i++) {
            char c = str[i];
            if (c >= '0' && c <= '9') {
                result = result * 10 + (c - '0');
            }
        }
    }
    
    if (negative) {
        return (uint32_t)(-(int32_t)result);
    }
    return result;
}

// parse register number from string like "R1"
int parse_register(const char* str) {
    if (str[0] == 'R') {
        int reg_num = atoi(str + 1);
        // accept R1-R6
        if (reg_num >= 1 && reg_num <= 6) {
            return reg_num;
        }
    }
    return -1;
}

// skip whitespace and line numbers, return pointer to instruction
char* skip_line_prefix(char* line) {
    char* ptr = line;
    
    // skip leading whitespace
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    
    // skip line numbers (digits at start)
    while (*ptr >= '0' && *ptr <= '9') {
        ptr++;
    }
    
    // skip whitespace after line number
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    
    return ptr;
}

// optimized instruction execution
void execute_instruction(char* line) {
    uint32_t cycles = 1;
    stats.total_instructions++;
    
    // skip line prefix (line numbers, whitespace)
    char* instruction = skip_line_prefix(line);
    
    // parse instruction type
    if (strncmp(instruction, "MOV", 3) == 0) {
        // mov r1, 5 or mov r1, r2
        char* comma = strchr(instruction, ',');
        if (comma) {
            char* reg_str = instruction + 3;
            while (*reg_str == ' ') reg_str++;
            
            char* value_str = comma + 1;
            while (*value_str == ' ') value_str++;
            
            int reg = parse_register(reg_str);
            if (reg >= 1 && reg <= 6) {
                if (value_str[0] == 'R') {
                    // mov r1, r2
                    int src_reg = parse_register(value_str);
                    if (src_reg >= 1 && src_reg <= 6) {
                        cpu.registers[reg] = cpu.registers[src_reg];
                    }
                } else {
                    // mov r1, 5
                    // validate 8-bit range [-128, 127]
                    int value = atoi(value_str);
                    if (value < -128) value = -128;
                    if (value > 127) value = 127;
                    cpu.registers[reg] = (int8_t)value;
                }
            }
        }
    }
    else if (strncmp(instruction, "ADD", 3) == 0) {
        // add r1, 5 or add r1, r2
        char* comma = strchr(instruction, ',');
        if (comma) {
            char* reg_str = instruction + 3;
            while (*reg_str == ' ') reg_str++;
            
            char* value_str = comma + 1;
            while (*value_str == ' ') value_str++;
            
            int reg = parse_register(reg_str);
            if (reg >= 1 && reg <= 6) {
                if (value_str[0] == 'R') {
                    // add r1, r2
                    int src_reg = parse_register(value_str);
                    if (src_reg >= 1 && src_reg <= 6) {
                        cpu.registers[reg] += cpu.registers[src_reg];
                    }
                } else {
                    // add r1, 5
                    int value = atoi(value_str);
                    if (value < -128) value = -128;
                    if (value > 127) value = 127;
                    cpu.registers[reg] += (int8_t)value;
                }
            }
        }
    }
    else if (strncmp(instruction, "CMP", 3) == 0) {
        // cmp r1, r2
        char* comma = strchr(instruction, ',');
        if (comma) {
            char* reg1_str = instruction + 3;
            while (*reg1_str == ' ') reg1_str++;
            
            char* reg2_str = comma + 1;
            while (*reg2_str == ' ') reg2_str++;
            
            int reg1 = parse_register(reg1_str);
            int reg2 = parse_register(reg2_str);
            
            if (reg1 >= 1 && reg1 <= 6 && reg2 >= 1 && reg2 <= 6) {
                cpu.zero_flag = (cpu.registers[reg1] == cpu.registers[reg2]);
            }
        }
    }
    else if (strncmp(instruction, "JE", 2) == 0) {
        // je 19 (jump if equal)
        char* target_str = instruction + 2;
        while (*target_str == ' ') target_str++;
        
        if (cpu.zero_flag) {
            int target_line = atoi(target_str);
            int target_index = find_instruction_index(target_line);
            if (target_index >= 0 && target_index < global_line_count) {
                cpu.pc = target_index;
                stats.total_cycles += cycles;
                return; // don't increment pc normally
            }
            // invalid jump target, halt!
            cpu.pc = global_line_count;
            stats.total_cycles += cycles;
            return;
        }
    }
    else if (strncmp(instruction, "JMP", 3) == 0) {
        // jmp 13 (unconditional jump)
        char* target_str = instruction + 3;
        while (*target_str == ' ') target_str++;
        
        int target_line = atoi(target_str);
        int target_index = find_instruction_index(target_line);
        if (target_index >= 0 && target_index < global_line_count) {
            cpu.pc = target_index;
            stats.total_cycles += cycles;
            return; // don't increment pc normally
        }
        // invalid jump target! halt!
        cpu.pc = global_line_count;
        stats.total_cycles += cycles;
        return;
    }
    else if (strncmp(instruction, "LD", 2) == 0) {
        // ld r5, [r3] or ld [r3], r5
        stats.ld_st_instructions++;
        
        char* comma = strchr(instruction, ',');
        if (comma) {
            char* reg_str = instruction + 2;
            while (*reg_str == ' ') reg_str++;
            
            // check if brackets come before comma (this is so not the best way to do this)
            char* bracket_start = strchr(reg_str, '[');
            if (bracket_start && bracket_start < comma) {
                // backwards -> LD [Rm], Rn
                char* bracket_end = strchr(bracket_start + 1, ']');
                if (bracket_end) {
                    char* addr_str = bracket_start + 1;
                    while (*addr_str == ' ') addr_str++;
                    
                    char* dest_reg_str = comma + 1;
                    while (*dest_reg_str == ' ') dest_reg_str++;
                    
                    int dest_reg = parse_register(dest_reg_str);
                    if (dest_reg >= 1 && dest_reg <= 6) {
                        if (addr_str[0] == 'R') {
                            int addr_reg = parse_register(addr_str);
                            if (addr_reg >= 1 && addr_reg <= 6) {
                                uint32_t mem_address = (uint32_t)cpu.registers[addr_reg];
                                cycles += simulate_memory_access(mem_address);
                                cpu.registers[dest_reg] = (int8_t)mem_address; // simplified
                            }
                        } else {
                            uint32_t mem_address = atoi(addr_str);
                            cycles += simulate_memory_access(mem_address);
                            cpu.registers[dest_reg] = (int8_t)mem_address; // simplified
                        }
                    }
                }
            } else {
                // Correct format: LD Rn, [Rm]
                bracket_start = strchr(comma, '[');
                if (bracket_start) {
                    char* bracket_end = strchr(bracket_start + 1, ']');
                    if (bracket_end) {
                        char* addr_str = bracket_start + 1;
                        while (*addr_str == ' ') addr_str++;
                        
                        int reg = parse_register(reg_str);
                        if (reg >= 1 && reg <= 6) {
                            if (addr_str[0] == 'R') {
                                int addr_reg = parse_register(addr_str);
                                if (addr_reg >= 1 && addr_reg <= 6) {
                                    uint32_t mem_address = (uint32_t)cpu.registers[addr_reg];
                                    cycles += simulate_memory_access(mem_address);
                                    cpu.registers[reg] = (int8_t)mem_address; // simplified
                                }
                            } else {
                                uint32_t mem_address = atoi(addr_str);
                                cycles += simulate_memory_access(mem_address);
                                cpu.registers[reg] = (int8_t)mem_address; // simplified
                            }
                        }
                    }
                }
            }
        }
    }
    else if (strncmp(instruction, "ST", 2) == 0) {
        // st [r3], r1
        stats.ld_st_instructions++;
        
        char* bracket_start = strchr(instruction, '[');
        if (bracket_start) {
            char* bracket_end = strchr(bracket_start + 1, ']');
            if (bracket_end) {
                char* comma = strchr(bracket_end, ',');
                if (comma) {
                    char* addr_str = bracket_start + 1;
                    while (*addr_str == ' ') addr_str++;
                    
                    char* reg_str = comma + 1;
                    while (*reg_str == ' ') reg_str++;
                    
                    int reg = parse_register(reg_str);
                    if (reg >= 1 && reg <= 6) {
                        if (addr_str[0] == 'R') {
                            int addr_reg = parse_register(addr_str);
                            if (addr_reg >= 1 && addr_reg <= 6) {
                                uint32_t mem_address = (uint32_t)cpu.registers[addr_reg];
                                cycles += simulate_memory_access(mem_address);
                            }
                        } else {
                            uint32_t mem_address = atoi(addr_str);
                            cycles += simulate_memory_access(mem_address);
                        }
                    }
                }
            }
        }
    }
    
    // increment program counter for non-jump instructions
    cpu.pc++;
    stats.total_cycles += cycles;
}

// optimized file processing
void process_assembly_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(1);
    }
    
    // reset global arrays
    global_line_count = 0;
    
    // read all lines
    while (fgets(global_lines[global_line_count], sizeof(global_lines[global_line_count]), file) && global_line_count < 1000) {
        // remove newline efficiently
        int len = strlen(global_lines[global_line_count]);
        if (len > 0 && global_lines[global_line_count][len-1] == '\n') {
            global_lines[global_line_count][len-1] = '\0';
            len--;
        }
        
        // skip empty lines, comments, and labels quickly
        if (len == 0 || global_lines[global_line_count][0] == '#' || global_lines[global_line_count][0] == ';' || global_lines[global_line_count][len-1] == ':') {
            continue;
        }
        
        // extract line number from the beginning of the line
        char* ptr = global_lines[global_line_count];
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        int line_num = atoi(ptr);
        
        // If no explicit line number found, use 1-based line numbering
        if (line_num == 0) {
            line_num = global_line_count + 1; // 1-based line number
        }
        global_line_numbers[global_line_count] = line_num;
        
        global_line_count++;
    }
    
    fclose(file);
    
    // execute instructions using program counter with reasonable limit
    cpu.pc = 0;
    uint32_t instruction_limit = 100000; // reasonable limit for performance testing
    uint32_t instruction_count = 0;
    
    while (cpu.pc < (uint32_t)global_line_count && instruction_count < instruction_limit) {
        execute_instruction(global_lines[cpu.pc]);
        instruction_count++;
    }
}

// optimized results printing
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
