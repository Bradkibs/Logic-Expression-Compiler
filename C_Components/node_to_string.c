#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to convert a node to a string representation
char* node_to_string(Node* node) {
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
            left_str = node_to_string(node->left);
            if (left_str) {
                result = malloc(strlen(left_str) + 5); // NOT + left + null terminator
                if (result) {
                    sprintf(result, "NOT %s", left_str);
                }
                free(left_str);
            }
            break;
            
        case NODE_AND:
            left_str = node_to_string(node->left);
            right_str = node_to_string(node->right);
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
            left_str = node_to_string(node->left);
            right_str = node_to_string(node->right);
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
            left_str = node_to_string(node->left);
            right_str = node_to_string(node->right);
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
            left_str = node_to_string(node->left);
            right_str = node_to_string(node->right);
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
            left_str = node_to_string(node->left);
            right_str = node_to_string(node->right);
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
                right_str = node_to_string(node->right);
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
    
    return result;
}
