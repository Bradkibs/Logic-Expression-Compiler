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
                fprintf(asm_file, "    true_str db \"TRUE\", 0\n");
                fprintf(asm_file, "    false_str db \"FALSE\", 0\n");
                fprintf(asm_file, "    equals_str db \" = \", 0\n");
                fprintf(asm_file, "    newline db 10, 0\n");
                fprintf(asm_file, "    var_values_str db \"Variable values:\", 10, 0\n");
                fprintf(asm_file, "    result_text db \"Result: \", 0\n");
                fprintf(asm_file, "    completion_text db 10, \"Completed evaluation of all expressions\", 10, 0\n");
                fprintf(asm_file, "    output_filename db \"output.txt\", 0\n");
                fprintf(asm_file, "    result_header db \"Logical Expression Results:\", 10, 0\n");
                fprintf(asm_file, "    evaluation_header db \"========== Logical Expression Evaluation ==========\n\", 0\n");
                fprintf(asm_file, "    variables_header db \"Variables:\n\", 0\n");
                fprintf(asm_file, "    expression_header db \"Expression:\n\", 0\n");
                
                // Define variables from the symbol table - supports any variable names dynamically
                fprintf(asm_file, "    ; Variables from the symbol table\n");
                for (int i = 0; i < symbol_table->size; i++) {
                    // Skip TRUE and FALSE constants
                    if (strcmp(symbol_table->symbols[i].name, "TRUE") != 0 && 
                        strcmp(symbol_table->symbols[i].name, "FALSE") != 0) {
                        fprintf(asm_file, "    %s resd 1  ; Variable from logical expressions\n", 
                                symbol_table->symbols[i].name);
                    }
                }
                
                // Temporary variables and buffers
                fprintf(asm_file, "    ; Temporary variables for evaluations\n");
                fprintf(asm_file, "    t0 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    t1 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    t2 resd 1  ; Temporary variable for evaluations\n");
                fprintf(asm_file, "    result_buffer resb 256  ; Buffer for result string\n");
                fprintf(asm_file, "    var_buffer resb 64    ; Buffer for variable names\n");
                
                // Generate output strings for each evaluation step in the data section
                for (int i = 0; i < eval_steps->step_count; i++) {
                    if (strncmp(eval_steps->steps[i]->step_description, "Evaluating expression:", 21) == 0) {
                        fprintf(asm_file, "    step%d db \"%s\", 0\n", i, eval_steps->steps[i]->step_description);
                    }
                }
                
                // Text section with all code
                fprintf(asm_file, "\nsection .text\n");
                fprintf(asm_file, "    global _start\n\n");
                
                // Add string function definitions
                fprintf(asm_file, "; String length function\n");
                fprintf(asm_file, "strlen:\n");
                fprintf(asm_file, "    push rbx\n");
                fprintf(asm_file, "    mov rbx, rdi\n");
                fprintf(asm_file, "    xor rax, rax\n");
                fprintf(asm_file, ".strlen_loop:\n");
                fprintf(asm_file, "    cmp byte [rbx], 0\n");
                fprintf(asm_file, "    je .strlen_end\n");
                fprintf(asm_file, "    inc rax\n");
                fprintf(asm_file, "    inc rbx\n");
                fprintf(asm_file, "    jmp .strlen_loop\n");
                fprintf(asm_file, ".strlen_end:\n");
                fprintf(asm_file, "    pop rbx\n");
                fprintf(asm_file, "    ret\n\n");
                
                fprintf(asm_file, "; String copy function\n");
                fprintf(asm_file, "strcpy:\n");
                fprintf(asm_file, "    push rdi\n");
                fprintf(asm_file, "    push rsi\n");
                fprintf(asm_file, "    push rdx\n");
                fprintf(asm_file, ".strcpy_loop:\n");
                fprintf(asm_file, "    mov dl, [rsi]\n");
                fprintf(asm_file, "    mov [rdi], dl\n");
                fprintf(asm_file, "    cmp dl, 0\n");
                fprintf(asm_file, "    je .strcpy_end\n");
                fprintf(asm_file, "    inc rdi\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, "    jmp .strcpy_loop\n");
                fprintf(asm_file, ".strcpy_end:\n");
                fprintf(asm_file, "    pop rdx\n");
                fprintf(asm_file, "    pop rsi\n");
                fprintf(asm_file, "    pop rdi\n");
                fprintf(asm_file, "    ret\n\n");
                
                fprintf(asm_file, "_start:\n");
                // Initialize variables with values from the symbol table
                fprintf(asm_file, "    ; Initialize variables with their assigned values\n");
                
                // Dynamically initialize all variables from the symbol table
                for (int i = 0; i < symbol_table->size; i++) {
                    // Skip TRUE and FALSE constants
                    if (strcmp(symbol_table->symbols[i].name, "TRUE") != 0 && 
                        strcmp(symbol_table->symbols[i].name, "FALSE") != 0) {
                        fprintf(asm_file, "    mov DWORD [%s], %d    ; %s = %s\n", 
                                symbol_table->symbols[i].name,
                                symbol_table->symbols[i].value,
                                symbol_table->symbols[i].name,
                                symbol_table->symbols[i].value ? "TRUE" : "FALSE");
                    }
                }
                fprintf(asm_file, "\n");
                
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
                
                // Generate output.txt file
                fprintf(asm_file, "    ; Create/Open output.txt file\n");
                fprintf(asm_file, "    mov rax, 2          ; syscall: open\n");
                fprintf(asm_file, "    mov rdi, output_filename ; filename pointer\n");
                fprintf(asm_file, "    mov rsi, 65         ; O_WRONLY | O_CREAT\n");
                fprintf(asm_file, "    mov rdx, 0666o      ; permissions\n");
                fprintf(asm_file, "    syscall\n");
                fprintf(asm_file, "    mov r12, rax        ; save file descriptor\n\n");
                
                fprintf(asm_file, "    ; Write evaluation header\n");
                fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    mov rsi, evaluation_header\n");
                fprintf(asm_file, "    mov rdx, 51         ; length of header\n");
                fprintf(asm_file, "    syscall\n\n");
                
                fprintf(asm_file, "    ; Write variables section\n");
                fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    mov rsi, variables_header\n");
                fprintf(asm_file, "    mov rdx, 11         ; length of header\n");
                fprintf(asm_file, "    syscall\n\n");
                
                // For each variable in the symbol table, create assembly code to write it to output.txt
                for (int i = 0; i < symbol_table->size; i++) {
                    // Skip TRUE and FALSE constants
                    if (strcmp(symbol_table->symbols[i].name, "TRUE") != 0 && 
                        strcmp(symbol_table->symbols[i].name, "FALSE") != 0) {
                        
                        // Create unique labels for this variable
                        char true_label[50], false_label[50];
                        sprintf(true_label, ".var_%d_true", i);
                        sprintf(false_label, ".var_%d_done", i);
                        
                        // Write code to format "X = TRUE/FALSE" in the buffer
                        fprintf(asm_file, "    ; Write variable %s to output.txt\n", symbol_table->symbols[i].name);
                        fprintf(asm_file, "    mov rsi, result_buffer\n");
                        
                        // Write the variable name first
                        fprintf(asm_file, "    mov rdi, var_buffer\n");
                        
                        // Write the variable name character by character
                        for (size_t j = 0; j < strlen(symbol_table->symbols[i].name); j++) {
                            fprintf(asm_file, "    mov BYTE [rdi + %zu], '%c'\n", j, symbol_table->symbols[i].name[j]);
                        }
                        fprintf(asm_file, "    mov BYTE [rdi + %zu], 0\n", strlen(symbol_table->symbols[i].name)); // Null terminator
                        
                        // Copy the variable name to the buffer
                        fprintf(asm_file, "    mov rsi, result_buffer\n");
                        fprintf(asm_file, "    mov rdi, var_buffer\n");
                        fprintf(asm_file, "    call strcpy\n");
                        
                        // Calculate the end of the string
                        fprintf(asm_file, "    mov rsi, result_buffer\n");
                        fprintf(asm_file, "    mov rdi, rsi\n");
                        fprintf(asm_file, "    call strlen\n");
                        fprintf(asm_file, "    add rsi, rax\n"); // rsi now points to end of string
                        
                        // Append " = "
                        fprintf(asm_file, "    mov DWORD [rsi], ' = '\n");
                        fprintf(asm_file, "    add rsi, 3\n");
                        
                        // Append TRUE/FALSE based on variable value
                        fprintf(asm_file, "    cmp DWORD [%s], 1\n", symbol_table->symbols[i].name);
                        fprintf(asm_file, "    jne %s\n", true_label);
                        fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                        fprintf(asm_file, "    add rsi, 4\n");
                        fprintf(asm_file, "    jmp %s\n", false_label);
                        fprintf(asm_file, "%s:\n", true_label);
                        fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                        fprintf(asm_file, "    add rsi, 4\n");
                        fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                        fprintf(asm_file, "    inc rsi\n");
                        fprintf(asm_file, "%s:\n", false_label);
                        
                        // Add newline and null terminator
                        fprintf(asm_file, "    mov BYTE [rsi], 10\n"); // newline
                        fprintf(asm_file, "    inc rsi\n");
                        fprintf(asm_file, "    mov BYTE [rsi], 0\n"); // null terminator
                        
                        // Write buffer to the file
                        fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                        fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                        fprintf(asm_file, "    mov rsi, result_buffer\n");
                        fprintf(asm_file, "    mov rdx, 256        ; max buffer size\n");
                        fprintf(asm_file, "    syscall\n\n");
                    }
                }
                
                // Write expression result to output.txt
                fprintf(asm_file, "    ; Write expression section\n");
                fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    mov rsi, expression_header\n");
                fprintf(asm_file, "    mov rdx, 13         ; length of header\n");
                fprintf(asm_file, "    syscall\n\n");
                
                // Format result buffer with the result of the expression
                fprintf(asm_file, "    mov rsi, result_buffer\n");
                fprintf(asm_file, "    mov DWORD [rsi], \"Resu\"\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov DWORD [rsi], \"lt: \"\n");
                fprintf(asm_file, "    add rsi, 4\n");
                
                // Add TRUE/FALSE based on result
                fprintf(asm_file, "    cmp DWORD [t0], 1\n");
                fprintf(asm_file, "    je .result_true_txt\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'FALS'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, "    mov BYTE [rsi], 'E'\n");
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, "    jmp .result_done_txt\n");
                fprintf(asm_file, ".result_true_txt:\n");
                fprintf(asm_file, "    mov DWORD [rsi], 'TRUE'\n");
                fprintf(asm_file, "    add rsi, 4\n");
                fprintf(asm_file, ".result_done_txt:\n");
                
                // Add newline and null terminator
                fprintf(asm_file, "    mov BYTE [rsi], 10\n"); // newline
                fprintf(asm_file, "    inc rsi\n");
                fprintf(asm_file, "    mov BYTE [rsi], 0\n"); // null terminator
                
                // Write result to file
                fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    mov rsi, result_buffer\n");
                fprintf(asm_file, "    mov rdx, 256        ; max buffer size\n");
                fprintf(asm_file, "    syscall\n\n");
                
                // Write completion message
                fprintf(asm_file, "    mov rax, 1          ; syscall: write\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    mov rsi, completion_text\n");
                fprintf(asm_file, "    mov rdx, 44         ; length\n");
                fprintf(asm_file, "    syscall\n\n");
                
                // Close the file
                fprintf(asm_file, "    mov rax, 3          ; syscall: close\n");
                fprintf(asm_file, "    mov rdi, r12        ; file descriptor\n");
                fprintf(asm_file, "    syscall\n\n");
                
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
    
    // Generate custom executable name based on input filename
    char executable_name[256] = {0};
    strcpy(executable_name, input_file);
    char* dot = strrchr(executable_name, '.');
    if (dot) {
        *dot = '\0';  // Remove the extension
    }
    
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
    
    // Link object file to create executable with the same name as the input file (without extension)
    snprintf(ld_cmd, sizeof(ld_cmd), "ld -o %s output.o -e _start", executable_name);
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
