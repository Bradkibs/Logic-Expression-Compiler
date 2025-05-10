#ifndef ASSEMBLY_GEN_H
#define ASSEMBLY_GEN_H

#include "intermediate_code_gen.h"

// Target architecture types
typedef enum {
    ARCH_X86,
    ARCH_X86_64,
    ARCH_ARM
} TargetArchitecture;

// Assembly generation configuration
typedef struct {
    char* output_filename;
    TargetArchitecture target_arch;
    int optimization_level;
} AssemblyGenConfig;

// Function prototypes
int generate_nasm_assembly(IntermediateCode* tac_code, AssemblyGenConfig* config);
void remove_redundant_moves(char** assembly_code, int* code_size);
void combine_logical_ops(char** assembly_code, int* code_size);
void peephole_optimize(char** assembly_code, int* code_size);
void eliminate_dead_code(char** assembly_code, int* code_size);
void reorder_instructions(char** assembly_code, int* code_size);

void optimize_assembly_code(char** assembly_code, int* code_size, int optimization_level);

#endif // ASSEMBLY_GEN_H
