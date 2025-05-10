#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assembly_gen.h"

int generate_nasm_assembly(IntermediateCode* tac_code, AssemblyGenConfig* config) {
    if (!tac_code || !config) return -1;

    FILE* output_file = fopen(config->output_filename, "w");
    if (!output_file) return -1;

    // NASM x86-64 assembly template
    fprintf(output_file, "section .data\n");
    fprintf(output_file, "    true db 'true', 0\n");
    fprintf(output_file, "    false db 'false', 0\n");
    
    // Define temporary variables used in three-address code
    // First, collect all variable names from the TAC
    char** var_names = malloc(100 * sizeof(char*));
    int var_count = 0;
    
    for (int i = 0; i < tac_code->count; i++) {
        TACInstruction* instr = tac_code->instructions[i];
        
        // Add result variable
        if (instr->result) {
            int exists = 0;
            for (int j = 0; j < var_count; j++) {
                if (strcmp(var_names[j], instr->result) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                var_names[var_count++] = strdup(instr->result);
            }
        }
        
        // Add arg1 variable
        if (instr->arg1) {
            int exists = 0;
            for (int j = 0; j < var_count; j++) {
                if (strcmp(var_names[j], instr->arg1) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists && instr->arg1[0] == 't') {
                var_names[var_count++] = strdup(instr->arg1);
            }
        }
        
        // Add arg2 variable
        if (instr->arg2) {
            int exists = 0;
            for (int j = 0; j < var_count; j++) {
                if (strcmp(var_names[j], instr->arg2) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists && instr->arg2[0] == 't') {
                var_names[var_count++] = strdup(instr->arg2);
            }
        }
    }
    
    // Define all collected variables in .data section
    for (int i = 0; i < var_count; i++) {
        fprintf(output_file, "    %s resd 1  ; Reserve a doubleword (4 bytes)\n", var_names[i]);
    }
    fprintf(output_file, "\n");

    fprintf(output_file, "section .text\n");
    fprintf(output_file, "    global _start\n\n");

    fprintf(output_file, "_start:\n");

    // Generate assembly for each three-address code instruction
    for (int i = 0; i < tac_code->count; i++) {
        TACInstruction* instr = tac_code->instructions[i];

        switch (instr->op) {
            case TAC_ASSIGN:
                fprintf(output_file, "    ; %s = %s\n", instr->result, instr->arg1);
                // Check if arg1 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                    fprintf(output_file, "    mov eax, %s\n", instr->arg1);
                } else {
                    fprintf(output_file, "    mov eax, [%s]\n", instr->arg1);
                }
                break;

            case TAC_NOT:
                fprintf(output_file, "    ; %s = NOT %s\n", instr->result, instr->arg1);
                // Check if arg1 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                    fprintf(output_file, "    mov eax, %s\n", instr->arg1);
                } else {
                    fprintf(output_file, "    mov eax, [%s]\n", instr->arg1);
                }
                fprintf(output_file, "    xor eax, 1  ; Bitwise NOT for boolean\n");
                break;

            case TAC_AND:
                fprintf(output_file, "    ; %s = %s AND %s\n", instr->result, instr->arg1, instr->arg2);
                // Check if arg1 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                    fprintf(output_file, "    mov eax, %s\n", instr->arg1);
                } else {
                    fprintf(output_file, "    mov eax, [%s]\n", instr->arg1);
                }
                // Check if arg2 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                    fprintf(output_file, "    and eax, %s\n", instr->arg2);
                } else {
                    fprintf(output_file, "    and eax, [%s]\n", instr->arg2);
                }
                break;

            case TAC_OR:
                fprintf(output_file, "    ; %s = %s OR %s\n", instr->result, instr->arg1, instr->arg2);
                // Check if arg1 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                    fprintf(output_file, "    mov eax, %s\n", instr->arg1);
                } else {
                    fprintf(output_file, "    mov eax, [%s]\n", instr->arg1);
                }
                // Check if arg2 is a literal (0 or 1) or a variable
                if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                    fprintf(output_file, "    or eax, %s\n", instr->arg2);
                } else {
                    fprintf(output_file, "    or eax, [%s]\n", instr->arg2);
                }
                break;

            // Add more cases for other logical operations
            default:
                break;
        }

        fprintf(output_file, "    mov [%s], eax\n\n", instr->result);
    }

    // Exit syscall
    fprintf(output_file, "    ; Exit program\n");
    fprintf(output_file, "    mov rax, 60     ; syscall number for exit\n");
    fprintf(output_file, "    xor rdi, rdi    ; exit code 0\n");
    fprintf(output_file, "    syscall\n");

    // Free memory allocated for variable names
    for (int i = 0; i < var_count; i++) {
        free(var_names[i]);
    }
    free(var_names);
    
    fclose(output_file);
    return 0;
}

void remove_redundant_moves(char** assembly_code, int* code_size) {
    // Scan through assembly and remove unnecessary mov instructions
    // e.g., mov rax, rax or mov rax, [rax] followed by mov [rax], rax
    if (!assembly_code || !*assembly_code || *code_size == 0) return;
    // Placeholder implementation
}

void combine_logical_ops(char** assembly_code, int* code_size) {
    // Look for consecutive AND, OR, XOR operations that can be combined
    // or simplified
    if (!assembly_code || !*assembly_code || *code_size == 0) return;
    // Placeholder implementation
}

void peephole_optimize(char** assembly_code, int* code_size) {
    // Replace complex instruction sequences with simpler equivalents
    // e.g., replace multiple instructions with a single more efficient one
    if (!assembly_code || !*assembly_code || *code_size == 0) return;
    // Placeholder implementation
}

void eliminate_dead_code(char** assembly_code, int* code_size) {
    // Remove instructions that do not affect the final program state
    // or are never reached
    if (!assembly_code || !*assembly_code || *code_size == 0) return;
    // Placeholder implementation
}

void reorder_instructions(char** assembly_code, int* code_size) {
    // Reorder instructions to improve cache locality
    // and reduce branch prediction misses
    if (!assembly_code || !*assembly_code || *code_size == 0) return;
    // Placeholder implementation
}

void optimize_assembly_code(char** assembly_code, int* code_size, int optimization_level) {
    if (!assembly_code || !*assembly_code || *code_size == 0) return;

    switch (optimization_level) {
        case 0:  // No optimization
            break;

        case 1:  // Basic optimization
            // Remove redundant move instructions
            remove_redundant_moves(assembly_code, code_size);
            // Combine consecutive logical operations
            combine_logical_ops(assembly_code, code_size);
            break;

        case 2:  // Aggressive optimization
            // Peephole optimizations
            peephole_optimize(assembly_code, code_size);
            // Dead code elimination
            eliminate_dead_code(assembly_code, code_size);
            // Instruction reordering for better cache performance
            reorder_instructions(assembly_code, code_size);
            break;

        default:
            break;
    }
}
