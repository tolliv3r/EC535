#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 256
#define CACHE_HIT_CYCLES 2
#define CACHE_MISS_CYCLES 50
#define LOCAL_MEMORY_SIZE 256

// Instruction types
typedef enum {
    MOV_REG_IMM = 0,
    MOV_REG_REG = 1,
    ADD_REG_REG = 2,
    ADD_REG_IMM = 3,
    CMP_REG_REG = 4,
    JE_ADDR = 5,
    JMP_ADDR = 6,
    LD_REG_REG = 7,
    ST_REG_REG = 8,
    LD_REV_REG_REG = 9,  // backward format: LD [Rm], Rn
    INVALID = 10
} InstructionType;

// Pre-decoded instruction
typedef struct {
    InstructionType type;
    int arg1;  // register number or address
    int arg2;  // register number or immediate value
} Instruction;

// Memory simulation
typedef struct {
    uint8_t memory[256];
    uint8_t touched[256];  // track which addresses have been accessed
} Memory;

// CPU simulation
typedef struct {
    int8_t registers[7]; // R1-R6 (index 1-6), index 0 unused
    uint8_t zero_flag; // for cmp instruction
} CPU;

// global variables
Memory memory = {0};
CPU cpu = {0};
Instruction* instructions = NULL;
int instruction_count = 0;
int first_line_number = 0;

// Parse integer from string
int parse_int(const char* str) {
    return atoi(str);
}

// Parse register number from string like "R1"
int parse_register(const char* str) {
    if (str[0] == 'R') {
        return atoi(str + 1);
    }
    return -1;
}

// Parse instruction from a line
Instruction parse_instruction_line(char* line) {
    Instruction inst = {INVALID, 0, 0};
    
    // Skip leading whitespace and line number
    char* ptr = line;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    while (*ptr >= '0' && *ptr <= '9') ptr++;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    // Parse instruction mnemonic
    if (strncmp(ptr, "MOV", 3) == 0) {
        ptr += 3;
        while (*ptr == ' ') ptr++;
        
        int reg = parse_register(ptr);
        if (reg >= 1 && reg <= 6) {
            char* comma = strchr(ptr, ',');
            if (comma) {
                comma++;
                while (*comma == ' ') comma++;
                
                if (comma[0] == 'R') {
                    // MOV Rn, Rm - treat as MOV with immediate from register
                    int src_reg = parse_register(comma);
                    if (src_reg >= 1 && src_reg <= 6) {
                        inst.type = MOV_REG_IMM;
                        inst.arg1 = reg;
                        inst.arg2 = 0; // will be resolved at execution
                    }
                } else {
                    inst.type = MOV_REG_IMM;
                    inst.arg1 = reg;
                    inst.arg2 = parse_int(comma);
                }
            }
        }
    }
    else if (strncmp(ptr, "ADD", 3) == 0) {
        ptr += 3;
        while (*ptr == ' ') ptr++;
        
        int reg = parse_register(ptr);
        if (reg >= 1 && reg <= 6) {
            char* comma = strchr(ptr, ',');
            if (comma) {
                comma++;
                while (*comma == ' ') comma++;
                
                if (comma[0] == 'R') {
                    inst.type = ADD_REG_REG;
                    inst.arg1 = reg;
                    inst.arg2 = parse_register(comma);
                } else {
                    inst.type = ADD_REG_IMM;
                    inst.arg1 = reg;
                    inst.arg2 = parse_int(comma);
                }
            }
        }
    }
    else if (strncmp(ptr, "CMP", 3) == 0) {
        ptr += 3;
        while (*ptr == ' ') ptr++;
        
        int reg1 = parse_register(ptr);
        if (reg1 >= 1 && reg1 <= 6) {
            char* comma = strchr(ptr, ',');
            if (comma) {
                comma++;
                while (*comma == ' ') comma++;
                
                int reg2 = parse_register(comma);
                if (reg2 >= 1 && reg2 <= 6) {
                    inst.type = CMP_REG_REG;
                    inst.arg1 = reg1;
                    inst.arg2 = reg2;
                }
            }
        }
    }
    else if (strncmp(ptr, "JE", 2) == 0) {
        ptr += 2;
        while (*ptr == ' ') ptr++;
        
        inst.type = JE_ADDR;
        inst.arg1 = parse_int(ptr);
        inst.arg2 = 0;
    }
    else if (strncmp(ptr, "JMP", 3) == 0) {
        ptr += 3;
        while (*ptr == ' ') ptr++;
        
        inst.type = JMP_ADDR;
        inst.arg1 = parse_int(ptr);
        inst.arg2 = 0;
    }
    else if (strncmp(ptr, "LD", 2) == 0) {
        ptr += 2;
        while (*ptr == ' ') ptr++;
        
        // Check if brackets come before comma (backward format)
        char* comma = strchr(ptr, ',');
        char* bracket_start = strchr(ptr, '[');
        
        if (comma && bracket_start && bracket_start < comma) {
            // LD [Rm], Rn - backward format
            bracket_start++;
            while (*bracket_start == ' ') bracket_start++;
            
            int addr_reg = parse_register(bracket_start);
            if (addr_reg >= 1 && addr_reg <= 6) {
                comma++;
                while (*comma == ' ') comma++;
                
                int dest_reg = parse_register(comma);
                if (dest_reg >= 1 && dest_reg <= 6) {
                    inst.type = LD_REV_REG_REG;
                    inst.arg1 = addr_reg;
                    inst.arg2 = dest_reg;
                }
            }
        } else {
            // LD Rn, [Rm] - normal format
            int reg = parse_register(ptr);
            if (reg >= 1 && reg <= 6) {
                comma = strchr(ptr, ',');
                if (comma) {
                    bracket_start = strchr(comma, '[');
                    if (bracket_start) {
                        bracket_start++;
                        while (*bracket_start == ' ') bracket_start++;
                        
                        int addr_reg = parse_register(bracket_start);
                        if (addr_reg >= 1 && addr_reg <= 6) {
                            inst.type = LD_REG_REG;
                            inst.arg1 = reg;
                            inst.arg2 = addr_reg;
                        }
                    }
                }
            }
        }
    }
    else if (strncmp(ptr, "ST", 2) == 0) {
        ptr += 2;
        while (*ptr == ' ') ptr++;
        
        char* bracket_start = strchr(ptr, '[');
        if (bracket_start) {
            bracket_start++;
            while (*bracket_start == ' ') bracket_start++;
            
            int addr_reg = parse_register(bracket_start);
            if (addr_reg >= 1 && addr_reg <= 6) {
                char* bracket_end = strchr(bracket_start, ']');
                if (bracket_end) {
                    char* comma = strchr(bracket_end, ',');
                    if (comma) {
                        comma++;
                        while (*comma == ' ') comma++;
                        
                        int reg = parse_register(comma);
                        if (reg >= 1 && reg <= 6) {
                            inst.type = ST_REG_REG;
                            inst.arg1 = addr_reg;
                            inst.arg2 = reg;
                        }
                    }
                }
            }
        }
    }
    
    return inst;
}

// Get instructions from file
Instruction* get_instructions_from_file(FILE* file, int* line_count, int* first_line) {
    char buffer[MAX_LINE_LENGTH];
    
    // Read first line to get starting line number
    if (fgets(buffer, sizeof(buffer), file)) {
        sscanf(buffer, "%d ", first_line);
    }
    rewind(file);
    
    // Count lines
    *line_count = 0;
    while (fgets(buffer, sizeof(buffer), file)) {
        (*line_count)++;
    }
    rewind(file);
    
    // Allocate instruction array
    Instruction* insts = malloc((*line_count + *first_line) * sizeof(Instruction));
    if (!insts) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Initialize all instructions to INVALID
    for (int i = 0; i < *line_count + *first_line; i++) {
        insts[i].type = INVALID;
        insts[i].arg1 = 0;
        insts[i].arg2 = 0;
    }
    
    // Parse instructions
    int idx = *first_line;
    while (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Skip empty lines, comments, and labels
        int len = strlen(buffer);
        if (len == 0 || buffer[0] == '#' || buffer[0] == ';') {
            continue;
        }
        
        // Skip lines ending with ':' (labels)
        if (buffer[len-1] == ':') {
            continue;
        }
        
        insts[idx] = parse_instruction_line(buffer);
        idx++;
    }
    
    return insts;
}

// Execute instructions
void execute_program() {
    int executed_instructions = 0;
    int clock_cycles = 0;
    int local_memory_hits = 0;
    int total_memory_hits = 0;
    
    // Initialize registers
    for (int i = 1; i < 7; i++) {
        cpu.registers[i] = 0;
    }
    
    // Execute instructions
    for (int i = first_line_number; i < instruction_count + first_line_number; i++) {
        executed_instructions++;
        Instruction inst = instructions[i];
        
        int cycles = 1;
        
        switch (inst.type) {
            case MOV_REG_IMM:
                cpu.registers[inst.arg1] = inst.arg2;
                cycles = 1;
                break;
                
            case MOV_REG_REG:
                cpu.registers[inst.arg1] = cpu.registers[inst.arg2];
                cycles = 1;
                break;
                
            case ADD_REG_REG:
                cpu.registers[inst.arg1] += cpu.registers[inst.arg2];
                cycles = 1;
                break;
                
            case ADD_REG_IMM:
                cpu.registers[inst.arg1] += inst.arg2;
                cycles = 1;
                break;
                
            case CMP_REG_REG:
                cpu.zero_flag = (cpu.registers[inst.arg1] == cpu.registers[inst.arg2]);
                cycles = 1;
                break;
                
            case JE_ADDR:
                if (cpu.zero_flag) {
                    int target = inst.arg1;
                    i = target - 1; // -1 because loop will increment
                }
                cycles = 1;
                break;
                
            case JMP_ADDR:
                {
                    int target = inst.arg1;
                    i = target - 1; // -1 because loop will increment
                }
                cycles = 1;
                break;
                
            case LD_REG_REG:
                {
                    uint8_t addr = (uint8_t)cpu.registers[inst.arg2];
                    if (!memory.touched[addr]) {
                        memory.touched[addr] = 1;
                        cycles = CACHE_MISS_CYCLES + 1;
                    } else {
                        local_memory_hits++;
                        cycles = CACHE_HIT_CYCLES + 1;
                    }
                    cpu.registers[inst.arg1] = memory.memory[addr];
                    total_memory_hits++;
                }
                break;
                
            case LD_REV_REG_REG:
                {
                    uint8_t addr = (uint8_t)cpu.registers[inst.arg1];
                    if (!memory.touched[addr]) {
                        memory.touched[addr] = 1;
                        cycles = CACHE_MISS_CYCLES + 1;
                    } else {
                        local_memory_hits++;
                        cycles = CACHE_HIT_CYCLES + 1;
                    }
                    cpu.registers[inst.arg2] = memory.memory[addr];
                    total_memory_hits++;
                }
                break;
                
            case ST_REG_REG:
                {
                    uint8_t addr = (uint8_t)cpu.registers[inst.arg1];
                    if (!memory.touched[addr]) {
                        memory.touched[addr] = 1;
                        cycles = CACHE_MISS_CYCLES + 1;
                    } else {
                        local_memory_hits++;
                        cycles = CACHE_HIT_CYCLES + 1;
                    }
                    memory.memory[addr] = (uint8_t)cpu.registers[inst.arg2];
                    total_memory_hits++;
                }
                break;
                
            case INVALID:
                // Skip invalid instructions
                cycles = 0;
                break;
        }
        
        clock_cycles += cycles;
    }
    
    // Print results
    printf("Total number of executed instructions: %d\n", executed_instructions);
    printf("Total number of clock cycles: %d\n", clock_cycles);
    printf("Number of hits to local memory: %d\n", local_memory_hits);
    printf("Total number of executed LD/ST instructions: %d\n", total_memory_hits);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <assembly_file>\n", argv[0]);
        exit(1);
    }
    
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        exit(1);
    }
    
    instructions = get_instructions_from_file(file, &instruction_count, &first_line_number);
    fclose(file);
    
    execute_program();
    
    free(instructions);
    return 0;
}
