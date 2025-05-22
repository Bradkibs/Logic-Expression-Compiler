#include <stdlib.h>
#include <string.h>
#include "semantic_analyzer.h"
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
