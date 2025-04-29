#ifndef AST_H
#define AST_H

typedef enum {
    NODE_VAR,
    NODE_ASSIGN,
    NODE_NOT,
    NODE_AND,
    NODE_OR,
    NODE_XOR,
    NODE_XNOR,
    NODE_IMPLIES,
    NODE_IFF,
    NODE_EQUIV,
    NODE_EXISTS,
    NODE_FORALL
} NodeType;

typedef struct Node {
    NodeType type;
    char* name;               // For variable or quantifier var
    struct Node* left;
    struct Node* right;
} Node;

// Constructors
Node* create_variable_node(char* name);
Node* create_assignment_node(char* name, Node* expr);
Node* create_not_node(Node* expr);
Node* create_and_node(Node* left, Node* right);
Node* create_or_node(Node* left, Node* right);
Node* create_xor_node(Node* left, Node* right);
Node* create_xnor_node(Node* left, Node* right);
Node* create_implies_node(Node* left, Node* right);
Node* create_iff_node(Node* left, Node* right);
Node* create_equiv_node(Node* left, Node* right);
Node* create_exists_node(char* var, Node* expr);
Node* create_forall_node(char* var, Node* expr);

void print_ast(Node* root, int indent);
void free_ast(Node* root);

#endif
