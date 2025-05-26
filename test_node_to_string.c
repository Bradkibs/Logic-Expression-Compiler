#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "C_Components/ast.h"

// Function to create a simple expression tree for testing
Node* create_test_expression() {
    // Create a test expression: (A AND B) OR (C AND D)
    Node* a = create_variable_node(strdup("A"));
    Node* b = create_variable_node(strdup("B"));
    Node* c = create_variable_node(strdup("C"));
    Node* d = create_variable_node(strdup("D"));
    
    // Create AND nodes
    Node* and1 = create_and_node(a, b);
    Node* and2 = create_and_node(c, d);
    
    // Create OR node
    Node* or_node = create_or_node(and1, and2);
    
    // Mark the OR node as parenthesized
    or_node->is_parenthesized = 1;
    
    return or_node;
}

void test_expression(const char* expr) {
    printf("Testing expression: %s\n", expr);
    
    // Parse the expression
    yy_scan_string(expr);
    parsed_expression = NULL;
    int parse_result = yyparse();
    
    if (parse_result != 0 || !parsed_expression) {
        printf("  Error: Failed to parse expression\n\n");
        return;
    }
    
    // Convert to string
    char* str = node_to_string(parsed_expression);
    if (str) {
        printf("  Result: %s\n", str);
        free(str);
    } else {
        printf("  Error: Failed to convert to string\n");
    }
    
    // Free the AST
    free_ast(parsed_expression);
    parsed_expression = NULL;
    
    printf("\n");
}

int main() {
    // Test cases
    test_expression("A AND B");
    test_expression("(A AND B)");
    test_expression("A AND (B OR C)");
    test_expression("(A AND B) OR C");
    test_expression("A AND B OR C");
    test_expression("NOT (A AND B)");
    test_expression("(A -> B) <-> C");
    
    return 0;
}
