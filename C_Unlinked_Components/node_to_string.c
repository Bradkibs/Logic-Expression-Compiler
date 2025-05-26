#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal function that handles precedence and parentheses
static char* node_to_string_internal(Node* node, int parent_precedence);

// Main function to convert a node to a string representation
char* node_to_string(Node* node) {
    // Start with lowest precedence (0) to avoid unnecessary parentheses at top level
    return node_to_string_internal(node, 0);
}

// Internal function that handles precedence and parentheses
static char* node_to_string_internal(Node* node, int parent_precedence) {
    if (!node) return NULL;
    
    char* left_str = NULL;
    char* right_str = NULL;
    char* result = NULL;
    
    // Define operator precedence (higher number = higher precedence)
    int precedence = 0;
    switch (node->type) {
        case NODE_NOT: precedence = 5; break;
        case NODE_AND: precedence = 4; break;
        case NODE_OR: precedence = 3; break;
        case NODE_XOR: precedence = 3; break;
        case NODE_IMPLIES: precedence = 2; break;
        case NODE_IFF:
        case NODE_EQUIV: precedence = 1; break;
        default: precedence = 0; break;
    }
    
    // Determine if we need parentheses
    int needs_parens = (node->is_parenthesized || (precedence > 0 && parent_precedence > precedence));
    
    switch (node->type) {
        case NODE_BOOL:
            result = strdup(node->bool_val ? "TRUE" : "FALSE");
            break;
            
        case NODE_VAR:
            result = strdup(node->name);
            break;
            
        case NODE_NOT:
            left_str = node_to_string_internal(node->left, precedence);
            if (left_str) {
                result = malloc(strlen(left_str) + 5); // NOT + left + null terminator
                if (result) {
                    sprintf(result, "NOT %s", left_str);
                }
                free(left_str);
            }
            break;
            
        case NODE_AND:
            left_str = node_to_string_internal(node->left, precedence);
            right_str = node_to_string_internal(node->right, precedence);
            if (left_str && right_str) {
                result = malloc(strlen(left_str) + strlen(right_str) + 6); // left + AND + right + null terminator
                if (result) {
                    sprintf(result, "%s AND %s", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_OR:
            left_str = node_to_string_internal(node->left, precedence);
            right_str = node_to_string_internal(node->right, precedence);
            if (left_str && right_str) {
                result = malloc(strlen(left_str) + strlen(right_str) + 5); // left + OR + right + null terminator
                if (result) {
                    sprintf(result, "%s OR %s", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_XOR:
            left_str = node_to_string_internal(node->left, precedence);
            right_str = node_to_string_internal(node->right, precedence);
            if (left_str && right_str) {
                result = malloc(strlen(left_str) + strlen(right_str) + 6); // left + XOR + right + null terminator
                if (result) {
                    sprintf(result, "%s XOR %s", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_IMPLIES:
            left_str = node_to_string_internal(node->left, precedence);
            right_str = node_to_string_internal(node->right, precedence);
            if (left_str && right_str) {
                result = malloc(strlen(left_str) + strlen(right_str) + 6); // left + -> + right + null terminator
                if (result) {
                    sprintf(result, "%s -> %s", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_IFF:
        case NODE_EQUIV:
            left_str = node_to_string_internal(node->left, precedence);
            right_str = node_to_string_internal(node->right, precedence);
            if (left_str && right_str) {
                result = malloc(strlen(left_str) + strlen(right_str) + 8); // left + <-> + right + null terminator
                if (result) {
                    sprintf(result, "%s <-> %s", left_str, right_str);
                }
                free(left_str);
                free(right_str);
            }
            break;
            
        case NODE_ASSIGN:
            if (node->left && node->left->type == NODE_VAR) {
                right_str = node_to_string_internal(node->right, 0);
                if (right_str) {
                    result = malloc(strlen(node->left->name) + strlen(right_str) + 4); // name + = + right + null terminator
                    if (result) {
                        sprintf(result, "%s = %s", node->left->name, right_str);
                    }
                    free(right_str);
                }
            }
            break;
            
        default:
            result = strdup("UNKNOWN");
            break;
    }
    
    // Add parentheses if needed
    if (needs_parens && result) {
        char* temp = result;
        result = malloc(strlen(temp) + 3); // ( + result + ) + null terminator
        if (result) {
            sprintf(result, "(%s)", temp);
        }
        free(temp);
    }
    
    return result;
}
