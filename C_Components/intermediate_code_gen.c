#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "intermediate_code_gen.h"

static char* generate_temp_var() {
    static int temp_counter = 0;
    char* temp_var = malloc(20 * sizeof(char));
    snprintf(temp_var, 20, "t%d", temp_counter++);
    return temp_var;
}

IntermediateCode* generate_three_address_code(Node* ast) {
    if (!ast) return NULL;

    IntermediateCode* code = malloc(sizeof(IntermediateCode));
    code->instructions = malloc(100 * sizeof(TACInstruction*));
    code->count = 0;
    code->capacity = 100;

    char* result = generate_temp_var();

    switch (ast->type) {
        case NODE_VAR:
            result = strdup(ast->name);
            break;

        case NODE_NOT: {
            char* arg = generate_three_address_code(ast->left)->instructions[0]->result;
            TACInstruction* instr = malloc(sizeof(TACInstruction));
            instr->op = TAC_NOT;
            instr->result = result;
            instr->arg1 = arg;
            instr->arg2 = NULL;
            code->instructions[code->count++] = instr;
            break;
        }

        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR:
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV: {
            // Generate code for left and right operands
            IntermediateCode* left_code = NULL;
            IntermediateCode* right_code = NULL;
            
            // Safely generate left operand code
            if (ast->left) {
                left_code = generate_three_address_code(ast->left);
                if (!left_code || left_code->count == 0) {
                    // Handle special case for variables and constants
                    if (ast->left->type == NODE_VAR) {
                        // Create a simple instruction to reference the variable
                        left_code = malloc(sizeof(IntermediateCode));
                        left_code->instructions = malloc(sizeof(TACInstruction*));
                        left_code->count = 1;
                        left_code->capacity = 1;
                        
                        TACInstruction* var_instr = malloc(sizeof(TACInstruction));
                        var_instr->op = TAC_ASSIGN;
                        var_instr->result = strdup(ast->left->name);
                        var_instr->arg1 = strdup(ast->left->name);
                        var_instr->arg2 = NULL;
                        left_code->instructions[0] = var_instr;
                    } else {
                        fprintf(stderr, "Error: Failed to generate left operand code\n");
                        free(result);
                        free(code->instructions);
                        free(code);
                        return NULL;
                    }
                }
            } else {
                fprintf(stderr, "Error: Missing left operand for binary operation\n");
                free(result);
                free(code->instructions);
                free(code);
                return NULL;
            }
            
            // Safely generate right operand code
            if (ast->right) {
                right_code = generate_three_address_code(ast->right);
                if (!right_code || right_code->count == 0) {
                    // Handle special case for variables and constants
                    if (ast->right->type == NODE_VAR) {
                        // Create a simple instruction to reference the variable
                        right_code = malloc(sizeof(IntermediateCode));
                        right_code->instructions = malloc(sizeof(TACInstruction*));
                        right_code->count = 1;
                        right_code->capacity = 1;
                        
                        TACInstruction* var_instr = malloc(sizeof(TACInstruction));
                        var_instr->op = TAC_ASSIGN;
                        var_instr->result = strdup(ast->right->name);
                        var_instr->arg1 = strdup(ast->right->name);
                        var_instr->arg2 = NULL;
                        right_code->instructions[0] = var_instr;
                    } else {
                        fprintf(stderr, "Error: Failed to generate right operand code\n");
                        free_intermediate_code(left_code);
                        free(result);
                        free(code->instructions);
                        free(code);
                        return NULL;
                    }
                }
            } else {
                fprintf(stderr, "Error: Missing right operand for binary operation\n");
                free_intermediate_code(left_code);
                free(result);
                free(code->instructions);
                free(code);
                return NULL;
            }

            // Determine operation type
            TACOpType op_type;
            switch (ast->type) {
                case NODE_AND: op_type = TAC_AND; break;
                case NODE_OR: op_type = TAC_OR; break;
                case NODE_XOR: op_type = TAC_XOR; break;
                case NODE_XNOR: op_type = TAC_XNOR; break;
                case NODE_IMPLIES: op_type = TAC_IMPLIES; break;
                case NODE_IFF: op_type = TAC_IFF; break;
                case NODE_EQUIV: op_type = TAC_EQUIV; break;
                default: op_type = TAC_ASSIGN; break;
            }

            // Create instruction with both operands
            TACInstruction* instr = malloc(sizeof(TACInstruction));
            instr->op = op_type;
            instr->result = result;
            instr->arg1 = strdup(left_code->instructions[0]->result);
            instr->arg2 = strdup(right_code->instructions[0]->result);
            code->instructions[code->count++] = instr;
            
            // Free intermediate code structures but keep the strings they reference
            // Don't free the result strings because we're using them
            free(left_code->instructions[0]);
            free(left_code->instructions);
            free(left_code);
            
            free(right_code->instructions[0]);
            free(right_code->instructions);
            free(right_code);
            break;
        }

        default:
            free(result);
            return NULL;
    }

    return code;
}

void free_intermediate_code(IntermediateCode* code) {
    if (!code) return;

    for (int i = 0; i < code->count; i++) {
        if (code->instructions[i]->result) {
            free(code->instructions[i]->result);
        }
        
        if (code->instructions[i]->arg1) {
            free(code->instructions[i]->arg1);
        }
        
        if (code->instructions[i]->arg2) {
            free(code->instructions[i]->arg2);
        }
        
        free(code->instructions[i]);
    }
    free(code->instructions);
    free(code);
}

void print_intermediate_code(IntermediateCode* code) {
    if (!code) return;

    for (int i = 0; i < code->count; i++) {
        TACInstruction* instr = code->instructions[i];
        printf("TAC: ");
        switch (instr->op) {
            case TAC_ASSIGN: printf("%s = %s", instr->result, instr->arg1); break;
            case TAC_NOT: printf("%s = NOT %s", instr->result, instr->arg1); break;
            case TAC_AND: printf("%s = %s AND %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_OR: printf("%s = %s OR %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_XOR: printf("%s = %s XOR %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_XNOR: printf("%s = %s XNOR %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_IMPLIES: printf("%s = %s IMPLIES %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_IFF: printf("%s = %s IFF %s", instr->result, instr->arg1, instr->arg2); break;
            case TAC_EQUIV: printf("%s = %s EQUIV %s", instr->result, instr->arg1, instr->arg2); break;
        }
        printf("\n");
    }
}
