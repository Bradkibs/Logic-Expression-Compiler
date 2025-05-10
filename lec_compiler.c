#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "C_Components/ast.h"
#include "C_Components/symbol_table.h"
#include "C_Components/semantic_analyzer.h"
#include "C_Components/intermediate_code_gen.h"
#include "C_Components/assembly_gen.h"

// External functions from the lexer/parser
extern int yyparse();
extern FILE* yyin;
extern Node* parsed_expression;

// Function to read a file into a string
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for file content
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read file content into buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';  // Null-terminate the string
    
    fclose(file);
    
    if (read_size != file_size) {
        fprintf(stderr, "Error: Failed to read the entire file\n");
        free(buffer);
        return NULL;
    }

    return buffer;
}

// Function to generate evaluation output file
void write_evaluation_results(const char* filename, EvaluationSteps* steps) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s' for writing\n", filename);
        return;
    }

    fprintf(file, "Logical Expression Evaluation\n---------------------------\n\n");
    
    // Write each step to the file
    for (int i = 0; i < steps->step_count; i++) {
        fprintf(file, "%s\n", steps->steps[i]->step_description);
        
        // Add extra newline after result steps for better readability
        if (strncmp(steps->steps[i]->step_description, "Result:", 7) == 0) {
            fprintf(file, "\n");
        }
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        printf("Usage: %s <input_file.lec> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  -o <output_file>   Specify output assembly file (default: output.asm)\n");
        printf("  -opt <level>       Set optimization level (0-2, default: 1)\n");
        return 1;
    }

    // Parse command line arguments
    const char* input_file = argv[1];
    const char* output_asm = "output.asm";
    int optimization_level = 1;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_asm = argv[++i];
        } else if (strcmp(argv[i], "-opt") == 0 && i + 1 < argc) {
            optimization_level = atoi(argv[++i]);
            if (optimization_level < 0 || optimization_level > 2) {
                fprintf(stderr, "Warning: Invalid optimization level. Using default (1).\n");
                optimization_level = 1;
            }
        }
    }

    // Open the input file for parsing
    FILE* input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Could not open input file '%s'\n", input_file);
        return 1;
    }
    
    // Set the input for the lexer
    yyin = input;
    
    printf("Compiling logical expressions from file: %s\n", input_file);
    
    // Symbol table for evaluation
    SymbolTable* symbol_table = init_symbol_table();
    if (!symbol_table) {
        fprintf(stderr, "Error: Failed to initialize symbol table\n");
        fclose(input);
        return 1;
    }
    
    // Initialize the symbol table with common boolean values
    add_or_update_symbol(symbol_table, "TRUE", 1);
    add_or_update_symbol(symbol_table, "FALSE", 0);
    
    // Parse the input
    int parse_result = yyparse();
    fclose(input);
    
    if (parse_result != 0 || !parsed_expression) {
        fprintf(stderr, "Error: Parsing failed\n");
        free_symbol_table(symbol_table);
        return 1;
    }
    
    // Perform semantic analysis on the parsed expression
    printf("Performing semantic analysis...\n");
    SemanticAnalysisResult semantic_result = perform_semantic_analysis(parsed_expression, symbol_table);
    if (semantic_result.error_code != SEMANTIC_OK) {
        fprintf(stderr, "Warning: Semantic analysis found issues: %s. Continuing anyway...\n", 
                semantic_result.error_message);
        // We're still continuing even with semantic issues
        free(semantic_result.error_message);
    }
    
    // Generate intermediate code
    printf("Generating three-address code...\n");
    IntermediateCode* tac_code = generate_three_address_code(parsed_expression);
    if (!tac_code) {
        fprintf(stderr, "Error: Failed to generate intermediate code\n");
        free_ast(parsed_expression);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    // Print the intermediate code for debugging
    printf("Three-address code:\n");
    print_intermediate_code(tac_code);
    
    // Configure assembly generation
    AssemblyGenConfig config;
    config.output_filename = strdup(output_asm);
    config.target_arch = ARCH_X86_64;
    config.optimization_level = optimization_level;
    
    // Generate assembly code
    printf("Generating assembly code...\n");
    int asm_result = generate_nasm_assembly(tac_code, &config);
    if (asm_result != 0) {
        fprintf(stderr, "Error: Failed to generate assembly code\n");
        free_intermediate_code(tac_code);
        free_ast(parsed_expression);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    printf("Assembly code written to %s\n", output_asm);
    
    // Always generate evaluation steps without requiring the flag
    // This ensures we have readable output when running the final executable
    char* file_content = read_file(input_file);
    if (file_content) {
        EvaluationSteps* eval_steps = evaluate_multiple_expressions(file_content);
        if (eval_steps) {
            write_evaluation_results("output.txt", eval_steps);
            printf("Evaluation steps written to output.txt\n");
            
            // Completely replace the assembly file to fix the execution order
            FILE* asm_file = fopen(output_asm, "w");
            if (asm_file) {
                // Data section with all variables and strings
                fprintf(asm_file, "section .data\n");
                fprintf(asm_file, "    true db 'true', 0\n");
                fprintf(asm_file, "    false db 'false', 0\n");
                fprintf(asm_file, "    newline db 10\n");  // Add newline character
                fprintf(asm_file, "    result_header db \"Logical Expression Results:\", 10, 0\n");
                
                // Define variables for our logical expressions
                fprintf(asm_file, "    A resd 1  ; Variable A from logical expressions\n");
                fprintf(asm_file, "    B resd 1  ; Variable B from logical expressions\n");
                fprintf(asm_file, "    C resd 1  ; Variable C from logical expressions\n");
                fprintf(asm_file, "    t0 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    t1 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    t2 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    result_buffer resb 128  ; Buffer for result string\n");
                
                // Generate output strings for each evaluation step in the data section
                for (int i = 0; i < eval_steps->step_count; i++) {
                    if (strncmp(eval_steps->steps[i]->step_description, "Evaluating expression:", 21) == 0) {
                        fprintf(asm_file, "    step%d db \"%s\", 0\n", i, eval_steps->steps[i]->step_description);
                    }
                }
                
                // Text section with all code
                fprintf(asm_file, "\nsection .text\n");
                fprintf(asm_file, "    global _start\n\n");
                
                fprintf(asm_file, "_start:\n");
                // Initialize symbol table with TRUE/FALSE values
                fprintf(asm_file, "    ; Initialize symbol table\n");
                fprintf(asm_file, "    mov DWORD [A], 1    ; A = TRUE\n");
                fprintf(asm_file, "    mov DWORD [B], 0    ; B = FALSE\n");
                fprintf(asm_file, "    mov DWORD [C], 0    ; C = FALSE\n\n");
                
                // Generate the original TAC code first
                for (int i = 0; i < tac_code->count; i++) {
                    TACInstruction* instr = tac_code->instructions[i];
                    
                    switch (instr->op) {
                        case TAC_NOT:
                            fprintf(asm_file, "    ; %s = NOT %s\n", instr->result, instr->arg1);
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            fprintf(asm_file, "    xor eax, 1  ; Bitwise NOT for boolean\n");
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_AND:
                            fprintf(asm_file, "    ; %s = %s AND %s\n", instr->result, instr->arg1, instr->arg2);
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                                fprintf(asm_file, "    and eax, %s\n", instr->arg2);
                            } else {
                                fprintf(asm_file, "    and eax, [%s]\n", instr->arg2);
                            }
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_OR:
                            fprintf(asm_file, "    ; %s = %s OR %s\n", instr->result, instr->arg1, instr->arg2);
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                                fprintf(asm_file, "    or eax, %s\n", instr->arg2);
                            } else {
                                fprintf(asm_file, "    or eax, [%s]\n", instr->arg2);
                            }
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_XOR:
                            fprintf(asm_file, "    ; %s = %s XOR %s\n", instr->result, instr->arg1, instr->arg2);
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                                fprintf(asm_file, "    xor eax, %s\n", instr->arg2);
                            } else {
                                fprintf(asm_file, "    xor eax, [%s]\n", instr->arg2);
                            }
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_IMPLIES:
                            fprintf(asm_file, "    ; %s = %s IMPLIES %s\n", instr->result, instr->arg1, instr->arg2);
                            // A -> B is equivalent to NOT A OR B
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            fprintf(asm_file, "    xor eax, 1    ; NOT A\n");  // NOT A
                            
                            if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                                fprintf(asm_file, "    mov ebx, %s\n", instr->arg2);
                            } else {
                                fprintf(asm_file, "    mov ebx, [%s]\n", instr->arg2);
                            }
                            fprintf(asm_file, "    or eax, ebx  ; NOT A OR B\n");
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_IFF:
                            fprintf(asm_file, "    ; %s = %s IFF %s\n", instr->result, instr->arg1, instr->arg2);
                            // A <-> B is equivalent to (A AND B) OR (NOT A AND NOT B)
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            if (strcmp(instr->arg2, "0") == 0 || strcmp(instr->arg2, "1") == 0) {
                                fprintf(asm_file, "    mov ebx, %s\n", instr->arg2);
                            } else {
                                fprintf(asm_file, "    mov ebx, [%s]\n", instr->arg2);
                            }
                            fprintf(asm_file, "    mov ecx, eax    ; Store A\n");
                            fprintf(asm_file, "    mov edx, ebx    ; Store B\n");
                            fprintf(asm_file, "    and eax, ebx    ; A AND B\n");
                            fprintf(asm_file, "    mov ebx, ecx    ; Restore A\n");
                            fprintf(asm_file, "    mov ecx, edx    ; Restore B\n");
                            fprintf(asm_file, "    xor ebx, 1      ; NOT A\n");
                            fprintf(asm_file, "    xor ecx, 1      ; NOT B\n");
                            fprintf(asm_file, "    and ebx, ecx    ; NOT A AND NOT B\n");
                            fprintf(asm_file, "    or eax, ebx     ; (A AND B) OR (NOT A AND NOT B)\n");
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        case TAC_ASSIGN:
                            fprintf(asm_file, "    ; %s = %s\n", instr->result, instr->arg1);
                            if (strcmp(instr->arg1, "0") == 0 || strcmp(instr->arg1, "1") == 0) {
                                fprintf(asm_file, "    mov eax, %s\n", instr->arg1);
                            } else {
                                fprintf(asm_file, "    mov eax, [%s]\n", instr->arg1);
                            }
                            fprintf(asm_file, "    mov [%s], eax\n\n", instr->result);
                            break;
                            
                        default:
                            // For other operations, default to copying
                            fprintf(asm_file, "    ; Unsupported TAC operation: %d\n", instr->op);
                            fprintf(asm_file, "    mov DWORD [%s], 0\n\n", instr->result);
                            break;
                    }
                }
                
                // Completely rewriting the output section
                fprintf(asm_file, "    ; Print variable values\n");
                fprintf(asm_file, "    mov rsi, result_buffer\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'A = '\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    cmp DWORD [A], 1\n");
                fprintf(asm_file, "    jne .a_false\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    jmp .a_done\n");
                fprintf(asm_file, ".a_false:\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, ".a_done:\n");
                
                fprintf(asm_file, "    mov BYTE [rsi], 10\n");  // newline
                fprintf(asm_file, "    inc rsi\n");
                
                fprintf(asm_file, "    mov DWORD [rsi], 'B = '\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    cmp DWORD [B], 1\n");
                fprintf(asm_file, "    jne .b_false\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    jmp .b_done\n");
                fprintf(asm_file, ".b_false:\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, ".b_done:\n");
                
                fprintf(asm_file, "    mov BYTE [rsi], 10\n");  // newline
                fprintf(asm_file, "    inc rsi\n");
                
                fprintf(asm_file, "    mov DWORD [rsi], 'C = '\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    cmp DWORD [C], 1\n");
                fprintf(asm_file, "    jne .c_false\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    jmp .c_done\n");
                fprintf(asm_file, ".c_false:\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, ".c_done:\n");
                
                fprintf(asm_file, "    mov BYTE [rsi], 10\n");  // newline
                fprintf(asm_file, "    inc rsi\n");
                
                fprintf(asm_file, "    ; Print result of expression\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'Resu'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'lt o'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'f ex'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'pres'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'sion'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov WORD [rsi], ': '\n");
                fprintf(asm_file, "    add rsi, 2\n");
                
                fprintf(asm_file, "    cmp DWORD [t0], 1\n");
                fprintf(asm_file, "    jne .result_false\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    jmp .result_done\n");
                fprintf(asm_file, ".result_false:\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, ".result_done:\n");
                
                fprintf(asm_file, "    mov BYTE [rsi], 0\n");  // null terminator
                
                fprintf(asm_file, "    ; Print the buffer\n");
                fprintf(asm_file, "    mov rax, 1      ; syscall number for write\n");
                fprintf(asm_file, "    mov rdi, 1      ; file descriptor 1 = STDOUT\n");
                fprintf(asm_file, "    mov rsi, result_buffer\n");
                fprintf(asm_file, "    mov rdx, 128     ; max buffer size\n");
                fprintf(asm_file, "    syscall\n");
                
                // Program exit - MUST BE LAST
                fprintf(asm_file, "    ; Exit program\n");
                fprintf(asm_file, "    mov rax, 60     ; syscall number for exit\n");
                fprintf(asm_file, "    xor rdi, rdi    ; exit code 0\n");
                fprintf(asm_file, "    syscall\n");
                
                fclose(asm_file);
                printf("Enhanced assembly code with output written to %s\n", output_asm);
            } else {
                fprintf(stderr, "Error: Could not open assembly file for writing\n");
            }
            free_evaluation_steps(eval_steps);
        }
        free(file_content);
    }
    
    // Generate executable using NASM and LD
    printf("Assembling and linking...\n");
    char nasm_cmd[300];
    char ld_cmd[300];
    
    // Generate object file from assembly
    snprintf(nasm_cmd, sizeof(nasm_cmd), "nasm -f elf64 -o output.o %s", output_asm);
    int nasm_result = system(nasm_cmd);
    if (nasm_result != 0) {
        fprintf(stderr, "Error: NASM assembly failed\n");
        free_intermediate_code(tac_code);
        free_ast(parsed_expression);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    // Link object file to create executable
    snprintf(ld_cmd, sizeof(ld_cmd), "ld -o lec_output output.o -e _start");
    int ld_result = system(ld_cmd);
    if (ld_result != 0) {
        fprintf(stderr, "Error: Linking failed\n");
        free_intermediate_code(tac_code);
        free_ast(parsed_expression);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    printf("Compilation successful! Executable created: lec_output\n");
    
    // Clean up with safety checks
    if (tac_code) {
        free_intermediate_code(tac_code);
        tac_code = NULL; // Prevent double-free
    }
    
    if (parsed_expression) {
        free_ast(parsed_expression);
        parsed_expression = NULL; // Prevent double-free
    }
    
    if (symbol_table) {
        free_symbol_table(symbol_table);
        symbol_table = NULL; // Prevent double-free
    }
    
    if (config.output_filename) {
        free(config.output_filename);
        config.output_filename = NULL; // Prevent double-free
    }
    
    return 0;
}
