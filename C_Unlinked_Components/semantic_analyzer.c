#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "semantic_analyzer.h"
#include "ast.h"
#include "symbol_table.h"

// Pre-process the AST to build the symbol table from assignments
void preprocess_symbol_table(Node* node, SymbolTable* symbol_table) {
    if (!node) return;
    
    // Process assignment nodes
    if (node->type == NODE_ASSIGN && node->name) {
        int value = 0;
        // If the right side is a boolean literal, use its value
        if (node->right && node->right->type == NODE_BOOL) {
            value = node->right->bool_val;
        }
        // Add the variable to the symbol table
        add_or_update_symbol(symbol_table, node->name, value);
    }
    
    // Recursively process child nodes
    if (node->left) preprocess_symbol_table(node->left, symbol_table);
    if (node->right) preprocess_symbol_table(node->right, symbol_table);
}

SemanticAnalysisResult perform_semantic_analysis(Node* ast, SymbolTable* symbol_table) {
    SemanticAnalysisResult result = {SEMANTIC_OK, NULL};

    if (!ast) {
        result.error_code = SEMANTIC_INVALID_QUANTIFIER;
        result.error_message = strdup("Invalid AST: NULL node");
        return result;
    }

    // First, preprocess the AST to build the symbol table from assignments
    preprocess_symbol_table(ast, symbol_table);
    
    // Validate variable usage
    if (!validate_variable_usage(ast, symbol_table)) {
        result.error_code = SEMANTIC_UNDEFINED_VARIABLE;
        result.error_message = strdup("Undefined variable used in expression");
        return result;
    }

    // Validate quantifier expressions
    if (!validate_quantifier_expression(ast, symbol_table)) {
        result.error_code = SEMANTIC_INVALID_QUANTIFIER;
        result.error_message = strdup("Invalid quantifier expression");
        return result;
    }
    
    // Check for ambiguous expressions that should have parentheses
    bool ambiguous = false;
    if (!check_ambiguous_expression(ast, &ambiguous) && ambiguous) {
        char* parenthesized = generate_parenthesized_expression(ast);
        if (parenthesized) {
            char* error_msg = malloc(strlen(parenthesized) + 100);
            if (error_msg) {
                sprintf(error_msg, "Ambiguous expression detected. Please use parentheses to clarify. Suggested: %s", parenthesized);
                result.error_code = SEMANTIC_AMBIGUOUS_EXPRESSION;
                result.error_message = error_msg;
            }
            free(parenthesized);
            return result;
        } else {
            result.error_code = SEMANTIC_AMBIGUOUS_EXPRESSION;
            result.error_message = strdup("Ambiguous expression detected. Please use parentheses to clarify precedence.");
            return result;
        }
    }

    return result;
}

bool validate_variable_usage(Node* node, SymbolTable* symbol_table) {
    if (!node) return true;

    switch (node->type) {
        case NODE_VAR:
            // Check if variable exists in symbol table using the refactored function
            if (get_symbol_value(symbol_table, node->name) == ERROR_SYMBOL_NOT_FOUND) {
                return false;
            }
            return true;

        case NODE_ASSIGN:
            // For assignments, add the variable to the symbol table and validate the right side
            if (node->right) {
                // Evaluate the right side first to ensure it's valid
                if (!validate_variable_usage(node->right, symbol_table)) {
                    return false;
                }
                
                // Determine the boolean value (assuming right node has been evaluated)
                int value = 0;
                if (node->right->type == NODE_BOOL) {
                    value = node->right->bool_val;
                }
                
                // Add or update the symbol in the table
                int result = add_or_update_symbol(symbol_table, node->name, value);
                if (result < 0) {
                    return false; // Failed to add to symbol table
                }
                return true;
            }
            return false;

        case NODE_NOT:
            return validate_variable_usage(node->left, symbol_table);

        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR:
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV:
            return validate_variable_usage(node->left, symbol_table) &&
                   validate_variable_usage(node->right, symbol_table);

        case NODE_EXISTS:
        case NODE_FORALL:
            // Add variable to symbol table for the quantified expression using refactored function
            if (add_or_update_symbol(symbol_table, node->name, 0) < 0) {
                return false;
            }
            return validate_variable_usage(node->left, symbol_table);

        case NODE_BOOL:
            // Boolean literals are always valid
            return true;

        default:
            return true;
    }
}

bool validate_quantifier_expression(Node* node, SymbolTable* symbol_table) {
    if (!node) return true;

    switch (node->type) {
        case NODE_EXISTS:
        case NODE_FORALL:
            // Ensure quantified variable is used in the expression
            if (!node->left || !node->name) return false;
            return true;

        case NODE_NOT:
            return validate_quantifier_expression(node->left, symbol_table);

        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR:
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV:
            return validate_quantifier_expression(node->left, symbol_table) &&
                   validate_quantifier_expression(node->right, symbol_table);

        default:
            return true;
    }
}

// Function to check if an expression is ambiguous (e.g. needs parentheses)
bool check_ambiguous_expression(Node* node, bool* ambiguous) {
    if (!node) return true;
    
    // Initialize ambiguous flag to false if not already set
    if (!ambiguous) return false;
    
    // If the node is explicitly parenthesized, it's not ambiguous
    if (node->is_parenthesized) {
        return true;
    }
    
    switch (node->type) {
        case NODE_VAR:
        case NODE_BOOL:
        case NODE_ASSIGN:
            // These are not ambiguous
            return true;
            
        case NODE_NOT:
            // For NOT nodes, only require parentheses around other operators
            // Allow NOT without parentheses for variables, booleans, and other NOTs
            if (node->left) {
                // If the left child is a binary operator, it needs parentheses
                if (node->left->type == NODE_AND || node->left->type == NODE_OR ||
                    node->left->type == NODE_XOR || node->left->type == NODE_XNOR ||
                    node->left->type == NODE_IMPLIES || node->left->type == NODE_IFF ||
                    node->left->type == NODE_EQUIV) {
                    *ambiguous = true;
                    return false;
                }
                // For other cases (variables, booleans, or other NOTs), continue checking
                return check_ambiguous_expression(node->left, ambiguous);
            }
            return true;
            
        case NODE_IMPLIES:
        case NODE_IFF:
        case NODE_EQUIV: {
            // These operators have lower precedence, so their operands might need parentheses
            bool left_ok = true;
            bool right_ok = true;
            
            // Check left child
            if (node->left) {
                // Left side needs parentheses if it's a binary operator with lower or equal precedence
                if (node->left->type == NODE_IMPLIES || node->left->type == NODE_IFF || 
                    node->left->type == NODE_EQUIV) {
                    *ambiguous = true;
                    return false;
                }
                left_ok = check_ambiguous_expression(node->left, ambiguous);
            }
            
            // Check right child
            if (node->right) {
                // Right side needs parentheses if it's a binary operator
                if (node->right->type == NODE_AND || node->right->type == NODE_OR ||
                    node->right->type == NODE_XOR || node->right->type == NODE_XNOR ||
                    node->right->type == NODE_IMPLIES || node->right->type == NODE_IFF ||
                    node->right->type == NODE_EQUIV) {
                    *ambiguous = true;
                    return false;
                }
                right_ok = check_ambiguous_expression(node->right, ambiguous);
            }
            
            return left_ok && right_ok;
        }
            
        case NODE_AND:
        case NODE_OR:
        case NODE_XOR:
        case NODE_XNOR: {
            // These operators have higher precedence
            bool left_ok = true;
            bool right_ok = true;
            
            // Check left child
            if (node->left) {
                // Left side needs parentheses if it's a binary operator with lower precedence
                if (node->left->type == NODE_IMPLIES || node->left->type == NODE_IFF || 
                    node->left->type == NODE_EQUIV) {
                    *ambiguous = true;
                    return false;
                }
                left_ok = check_ambiguous_expression(node->left, ambiguous);
            }
            
            // Check right child
            if (node->right) {
                // Right side needs parentheses if it's a binary operator with lower or equal precedence
                if (node->right->type == NODE_IMPLIES || node->right->type == NODE_IFF || 
                    node->right->type == NODE_EQUIV) {
                    *ambiguous = true;
                    return false;
                }
                right_ok = check_ambiguous_expression(node->right, ambiguous);
            }
            
            return left_ok && right_ok;
        }
            
        default:
            return true;
    }
}

// Function to generate a fully parenthesized expression string
char* generate_parenthesized_expression(Node* node) {
    if (!node) return NULL;
    
    char* left_str = NULL;
    char* right_str = NULL;
    char* result = NULL;
    
    switch (node->type) {
        case NODE_BOOL:
            result = strdup(node->bool_val ? "TRUE" : "FALSE");
            break;
            
        case NODE_VAR:
            result = strdup(node->name);
            break;
            
        case NODE_NOT:
            left_str = generate_parenthesized_expression(node->left);
            if (left_str) {
                // Always add parentheses around NOT expressions
                result = malloc(strlen(left_str) + 10); // NOT + ( + ) + null terminator
                if (result) {
                    sprintf(result, "NOT (%s)", left_str);
                }
                free(left_str);
            }
            break;
            
        case NODE_AND:
            left_str = generate_parenthesized_expression(node->left);
            right_str = generate_parenthesized_expression(node->right);
            if (left_str && right_str) {
                // Always add parentheses around AND expressions
                result = malloc(strlen(left_str) + strlen(right_str) + 10);
                if (result) {
                    sprintf(result, "(%s) AND (%s)", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_OR:
            left_str = generate_parenthesized_expression(node->left);
            right_str = generate_parenthesized_expression(node->right);
            if (left_str && right_str) {
                // Always add parentheses around OR expressions
                result = malloc(strlen(left_str) + strlen(right_str) + 10);
                if (result) {
                    sprintf(result, "(%s) OR (%s)", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_XOR:
            left_str = generate_parenthesized_expression(node->left);
            right_str = generate_parenthesized_expression(node->right);
            if (left_str && right_str) {
                // Always add parentheses around XOR expressions
                result = malloc(strlen(left_str) + strlen(right_str) + 10);
                if (result) {
                    sprintf(result, "(%s) XOR (%s)", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_IMPLIES:
            left_str = generate_parenthesized_expression(node->left);
            right_str = generate_parenthesized_expression(node->right);
            if (left_str && right_str) {
                // Always add parentheses around IMPLIES expressions
                result = malloc(strlen(left_str) + strlen(right_str) + 10);
                if (result) {
                    sprintf(result, "(%s) -> (%s)", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_IFF:
        case NODE_EQUIV:
            left_str = generate_parenthesized_expression(node->left);
            right_str = generate_parenthesized_expression(node->right);
            if (left_str && right_str) {
                // Always add parentheses around IFF/EQUIV expressions
                result = malloc(strlen(left_str) + strlen(right_str) + 12);
                if (result) {
                    sprintf(result, "(%s) <-> (%s)", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_ASSIGN:
            if (node->left && node->left->type == NODE_VAR) {
                right_str = generate_parenthesized_expression(node->right);
                if (right_str) {
                    result = malloc(strlen(node->left->name) + strlen(right_str) + 5);
                    if (result) {
                        sprintf(result, "%s = %s", node->left->name, right_str);
                    }
                    free(right_str);
                }
            }
            break;
            
        default:
            // For other node types, return a default string
            result = strdup("UNKNOWN");
            break;
    }
    
    return result;
}
