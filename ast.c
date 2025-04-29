#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

Node* create_node(NodeType type, char* name, Node* left, Node* right) {
    Node* node = malloc(sizeof(Node));
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->left = left;
    node->right = right;
    return node;
}

Node* create_variable_node(char* name) { return create_node(NODE_VAR, name, NULL, NULL); }
Node* create_assignment_node(char* name, Node* expr) { return create_node(NODE_ASSIGN, name, expr, NULL); }
Node* create_not_node(Node* expr) { return create_node(NODE_NOT, NULL, expr, NULL); }
Node* create_and_node(Node* l, Node* r) { return create_node(NODE_AND, NULL, l, r); }
Node* create_or_node(Node* l, Node* r) { return create_node(NODE_OR, NULL, l, r); }
Node* create_xor_node(Node* l, Node* r) { return create_node(NODE_XOR, NULL, l, r); }
Node* create_xnor_node(Node* l, Node* r) { return create_node(NODE_XNOR, NULL, l, r); }
Node* create_implies_node(Node* l, Node* r) { return create_node(NODE_IMPLIES, NULL, l, r); }
Node* create_iff_node(Node* l, Node* r) { return create_node(NODE_IFF, NULL, l, r); }
Node* create_equiv_node(Node* l, Node* r) { return create_node(NODE_EQUIV, NULL, l, r); }
Node* create_exists_node(char* var, Node* expr) { return create_node(NODE_EXISTS, var, expr, NULL); }
Node* create_forall_node(char* var, Node* expr) { return create_node(NODE_FORALL, var, expr, NULL); }

void print_ast(Node* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("NodeType: %d", node->type);
    if (node->name) printf(", Name: %s", node->name);
    printf("\n");
    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
}

void free_ast(Node* node) {
    if (!node) return;
    free(node->name);
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}
