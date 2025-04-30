#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"
#include "symbol_table.h"

// External declarations from lexer/parser
extern int yyparse(void);
Node *parsed_expression;
extern void yy_scan_string(const char *);
extern void yy_delete_buffer(void *);
extern void *yy_create_buffer(FILE *, int);
extern void yy_switch_to_buffer(void *);
extern int yydebug;

int yydebug = 1;

// Parser error handling
void yyerror(const char *s)
{
    fprintf(stderr, "Parser error: %s\n", s);
}

// General node creation function
Node *create_node(NodeType type, char *name, Node *left, Node *right, int bool_val)
{
    Node *node = malloc(sizeof(Node));
    if (!node)
    {
        fprintf(stderr, "Memory allocation failed in create_node\n");
        return NULL;
    }

    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->left = left;
    node->right = right;
    node->bool_val = bool_val;
    return node;
}

// Node creation wrappers
Node *create_variable_node(char *name) { return create_node(NODE_VAR, name, NULL, NULL, 0); }
Node *create_assignment_node(char *name, Node *expr) { return create_node(NODE_ASSIGN, name, expr, NULL, 0); }
Node *create_not_node(Node *expr) { return create_node(NODE_NOT, NULL, expr, NULL, 0); }
Node *create_and_node(Node *l, Node *r) { return create_node(NODE_AND, NULL, l, r, 0); }
Node *create_or_node(Node *l, Node *r) { return create_node(NODE_OR, NULL, l, r, 0); }
Node *create_xor_node(Node *l, Node *r) { return create_node(NODE_XOR, NULL, l, r, 0); }
Node *create_xnor_node(Node *l, Node *r) { return create_node(NODE_XNOR, NULL, l, r, 0); }
Node *create_implies_node(Node *l, Node *r) { return create_node(NODE_IMPLIES, NULL, l, r, 0); }
Node *create_iff_node(Node *l, Node *r) { return create_node(NODE_IFF, NULL, l, r, 0); }
Node *create_equiv_node(Node *l, Node *r) { return create_node(NODE_EQUIV, NULL, l, r, 0); }
Node *create_exists_node(char *var, Node *expr) { return create_node(NODE_EXISTS, var, expr, NULL, 0); }
Node *create_forall_node(char *var, Node *expr) { return create_node(NODE_FORALL, var, expr, NULL, 0); }
Node *create_boolean_node(int value) { return create_node(NODE_BOOL, NULL, NULL, NULL, value); }

// AST printing
void print_ast(Node *node, int indent)
{
    if (!node)
        return;
    for (int i = 0; i < indent; i++)
        printf("  ");
    printf("NodeType: %d", node->type);
    if (node->name)
        printf(", Name: %s", node->name);
    if (node->type == NODE_BOOL)
        printf(", Value: %s", node->bool_val ? "true" : "false");
    printf("\n");
    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
}

// Free memory of the AST - FIXED to prevent double-free issues
void free_ast(Node *node)
{
    if (!node)
        return;

    // Store temporary pointers to prevent accessing freed memory
    Node *left = node->left;
    Node *right = node->right;

    // Free the node's name if it exists
    if (node->name)
    {
        free(node->name);
        node->name = NULL;
    }

    // Free the node itself
    free(node);

    // Now recursively free children
    if (left)
        free_ast(left);
    if (right)
        free_ast(right);
}

// Deep copy of AST node
Node *clone_node(const Node *node)
{
    if (!node)
        return NULL;

    Node *left_copy = clone_node(node->left);
    Node *right_copy = clone_node(node->right);
    return create_node(node->type, node->name, left_copy, right_copy, node->bool_val);
}

// Logical Laws
Node *apply_de_morgan(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_NOT)
        return node;

    if (node->left && (node->left->type == NODE_AND || node->left->type == NODE_OR))
    {
        NodeType new_type = node->left->type == NODE_AND ? NODE_OR : NODE_AND;
        Node *new_left = create_not_node(clone_node(node->left->left));
        Node *new_right = create_not_node(clone_node(node->left->right));
        Node *transformed = create_node(new_type, NULL, new_left, new_right, 0);
        add_evaluation_step(steps, "Applied De Morgan's Law");

        // Free the original node but keep its children since we already cloned them
        Node *old_left = node->left;
        node->left = NULL; // Prevent recursive free
        free_ast(node);    // Free just this node

        // Free the original left node but prevent freeing its children
        Node *left_left = old_left->left;
        Node *left_right = old_left->right;
        old_left->left = NULL;
        old_left->right = NULL;
        free_ast(old_left);

        // Now free the original children
        if (left_left)
            free_ast(left_left);
        if (left_right)
            free_ast(left_right);

        return transformed;
    }
    return node;
}

Node *apply_commutative(Node *node, EvaluationSteps *steps)
{
    if (!node)
        return node;
    if (node->type == NODE_AND || node->type == NODE_OR)
    {
        Node *temp = node->left;
        node->left = node->right;
        node->right = temp;
        add_evaluation_step(steps, "Applied Commutative Law");
    }
    return node;
}

Node *apply_distributive(Node *node, EvaluationSteps *steps)
{
    if (!node || !node->left || !node->right)
        return node;
    if (node->type == NODE_AND && node->right->type == NODE_OR)
    {
        Node *a = clone_node(node->left);
        Node *b = clone_node(node->right->left);
        Node *c = clone_node(node->right->right);
        Node *left = create_and_node(a, b);
        Node *right = create_and_node(clone_node(node->left), c);
        Node *transformed = create_or_node(left, right);
        add_evaluation_step(steps, "Applied Distributive Law: AND over OR");

        // Free the original node structure
        Node *old_left = node->left;
        Node *old_right = node->right;
        node->left = NULL;
        node->right = NULL;
        free_ast(node);

        // Free the original children
        free_ast(old_left);
        free_ast(old_right);

        return transformed;
    }
    return node;
}

Node *apply_implication(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_IMPLIES)
        return node;
    Node *not_left = create_not_node(clone_node(node->left));
    Node *transformed = create_or_node(not_left, clone_node(node->right));
    add_evaluation_step(steps, "Applied Implication Law: A -> B == ~A OR B");

    // Clean up original node
    Node *old_left = node->left;
    Node *old_right = node->right;
    node->left = NULL;
    node->right = NULL;
    free_ast(node);
    free_ast(old_left);
    free_ast(old_right);

    return transformed;
}

Node *apply_iff(Node *node, EvaluationSteps *steps)
{
    if (!node || node->type != NODE_IFF)
        return node;
    Node *left_impl = create_implies_node(clone_node(node->left), clone_node(node->right));
    Node *right_impl = create_implies_node(clone_node(node->right), clone_node(node->left));
    Node *transformed = create_and_node(left_impl, right_impl);
    add_evaluation_step(steps, "Applied IFF Law: A <-> B == (A -> B) AND (B -> A)");

    // Clean up original node
    Node *old_left = node->left;
    Node *old_right = node->right;
    node->left = NULL;
    node->right = NULL;
    free_ast(node);
    free_ast(old_left);
    free_ast(old_right);

    return transformed;
}

// Apply logical laws with symbol table integration
Node *apply_logical_laws(Node *node, EvaluationSteps *steps)
{
    if (!node)
        return NULL;

    // First, handle direct evaluation of simple logical operations
    if (node->type == NODE_AND || node->type == NODE_OR || node->type == NODE_XOR || node->type == NODE_NOT)
    {
        if (node->type == NODE_AND)
        {
            if (node->left && node->right &&
                node->left->type == NODE_BOOL && node->right->type == NODE_BOOL)
            {
                int result = node->left->bool_val && node->right->bool_val;
                Node *result_node = create_boolean_node(result);
                add_evaluation_step(steps, "Evaluated AND operation");

                // Clean up original nodes safely
                Node *old_left = node->left;
                Node *old_right = node->right;
                node->left = NULL;
                node->right = NULL;
                free_ast(node);
                free_ast(old_left);
                free_ast(old_right);

                return result_node;
            }
        }
        else if (node->type == NODE_OR)
        {
            if (node->left && node->right &&
                node->left->type == NODE_BOOL && node->right->type == NODE_BOOL)
            {
                int result = node->left->bool_val || node->right->bool_val;
                Node *result_node = create_boolean_node(result);
                add_evaluation_step(steps, "Evaluated OR operation");

                // Clean up original nodes safely
                Node *old_left = node->left;
                Node *old_right = node->right;
                node->left = NULL;
                node->right = NULL;
                free_ast(node);
                free_ast(old_left);
                free_ast(old_right);

                return result_node;
            }
        }
        else if (node->type == NODE_XOR)
        {
            if (node->left && node->right &&
                node->left->type == NODE_BOOL && node->right->type == NODE_BOOL)
            {
                int result = (node->left->bool_val != node->right->bool_val);
                Node *result_node = create_boolean_node(result);
                add_evaluation_step(steps, "Evaluated XOR operation");

                // Clean up original nodes safely
                Node *old_left = node->left;
                Node *old_right = node->right;
                node->left = NULL;
                node->right = NULL;
                free_ast(node);
                free_ast(old_left);
                free_ast(old_right);

                return result_node;
            }
        }
        else if (node->type == NODE_NOT)
        {
            if (node->left && node->left->type == NODE_BOOL)
            {
                int result = !node->left->bool_val;
                Node *result_node = create_boolean_node(result);
                add_evaluation_step(steps, "Evaluated NOT operation");

                // Clean up original nodes safely
                Node *old_left = node->left;
                node->left = NULL;
                free_ast(node);
                free_ast(old_left);

                return result_node;
            }
        }
    }

    // Recurse first (post-order traversal)
    if (node->left)
    {
        Node *new_left = apply_logical_laws(node->left, steps);
        if (new_left != node->left)
        {
            node->left = new_left;
        }
    }

    if (node->right)
    {
        Node *new_right = apply_logical_laws(node->right, steps);
        if (new_right != node->right)
        {
            node->right = new_right;
        }
    }

    // Then apply transformations for complex cases
    switch (node->type)
    {
    case NODE_NOT:
        return apply_de_morgan(node, steps);
    case NODE_IMPLIES:
        return apply_implication(node, steps);
    case NODE_IFF:
        return apply_iff(node, steps);
    case NODE_AND:
    case NODE_OR:
        node = apply_commutative(node, steps);
        node = apply_distributive(node, steps);
        // Try to evaluate again after transformations
        if (node && node->left && node->right &&
            node->left->type == NODE_BOOL &&
            node->right->type == NODE_BOOL)
        {
            int result;
            if (node->type == NODE_AND)
                result = node->left->bool_val && node->right->bool_val;
            else // NODE_OR
                result = node->left->bool_val || node->right->bool_val;

            Node *result_node = create_boolean_node(result);
            add_evaluation_step(steps, "Evaluated logical operation after transformation");

            // Clean up original nodes safely
            Node *old_left = node->left;
            Node *old_right = node->right;
            node->left = NULL;
            node->right = NULL;
            free_ast(node);
            free_ast(old_left);
            free_ast(old_right);

            return result_node;
        }
        break;
    default:
        break;
    }

    return node;
}

// Evaluation with symbol table - FIXED to prevent memory corruption
Node *evaluate_node_with_symbol_table(Node *node, SymbolTable *symbol_table, EvaluationSteps *steps)
{
    if (!node)
        return NULL;

    // Handle variable references (replace with their stored boolean values)
    if (node->type == NODE_VAR)
    {
        bool value;
        char desc[256];
        if (get_symbol_value(symbol_table, node->name, &value))
        {
            snprintf(desc, sizeof(desc), "Substituted variable %s with value %s",
                     node->name, value ? "TRUE" : "FALSE");
            add_evaluation_step(steps, desc);

            // Create result node before freeing original
            Node *result = create_boolean_node(value);
            free_ast(node);
            return result;
        }
        else
        {
            snprintf(desc, sizeof(desc), "WARNING: Undefined variable %s", node->name);
            add_evaluation_step(steps, desc);
            return node;
        }
    }
    // Process assignments
    else if (node->type == NODE_ASSIGN)
    {
        char desc[256];
        // Don't evaluate the right side of the assignment directly
        // Instead evaluate a copy to avoid potential double freeing
        Node *value_node = NULL;
        if (node->left)
        {
            value_node = evaluate_node_with_symbol_table(clone_node(node->left), symbol_table, steps);
        }

        if (value_node && value_node->type == NODE_BOOL)
        {
            // Store the assignment in the symbol table
            add_or_update_symbol(symbol_table, node->name, value_node->bool_val);
            snprintf(desc, sizeof(desc), "Assigned %s = %s",
                     node->name, value_node->bool_val ? "TRUE" : "FALSE");
            add_evaluation_step(steps, desc);

            // Return the boolean result of the assignment
            Node *result = create_boolean_node(value_node->bool_val);

            // Free the evaluated copy
            free_ast(value_node);

            // Free the original node
            free_ast(node);

            return result;
        }
        else
        {
            // Could not evaluate the assignment
            snprintf(desc, sizeof(desc), "Failed to evaluate assignment for %s", node->name);
            add_evaluation_step(steps, desc);

            // Free the evaluated copy if it exists
            if (value_node)
            {
                free_ast(value_node);
            }

            return node;
        }
    }

    // For other node types, apply transformations
    // Create copies of children to avoid modifying the originals during evaluation
    Node *left_result = NULL;
    Node *right_result = NULL;

    if (node->left)
    {
        left_result = evaluate_node_with_symbol_table(clone_node(node->left), symbol_table, steps);
    }

    if (node->right)
    {
        right_result = evaluate_node_with_symbol_table(clone_node(node->right), symbol_table, steps);
    }

    // Create a new node with the evaluated children
    Node *new_node = create_node(node->type, node->name ? strdup(node->name) : NULL,
                                 left_result, right_result, node->bool_val);

    // Free the original node but not its children (we've cloned them)
    Node *temp_left = node->left;
    Node *temp_right = node->right;
    node->left = NULL;
    node->right = NULL;
    free_ast(node);

    // Now free the original children
    if (temp_left)
        free_ast(temp_left);
    if (temp_right)
        free_ast(temp_right);

    // Apply logical laws to the new node with evaluated children
    return apply_logical_laws(new_node, steps);
}

// Evaluation step helpers - FIXED
EvaluationSteps *init_evaluation_steps()
{
    EvaluationSteps *steps = malloc(sizeof(EvaluationSteps));
    if (!steps)
    {
        fprintf(stderr, "Memory allocation failed for evaluation steps\n");
        return NULL;
    }

    steps->steps = malloc(sizeof(EvaluationStep *) * 10);
    if (!steps->steps)
    {
        fprintf(stderr, "Memory allocation failed for evaluation steps array\n");
        free(steps);
        return NULL;
    }

    steps->step_count = 0;
    steps->capacity = 10;
    return steps;
}

void add_evaluation_step(EvaluationSteps *steps, const char *description)
{
    if (!steps || !description)
    {
        return;
    }

    if (steps->step_count >= steps->capacity)
    {
        size_t new_capacity = steps->capacity * 2;
        EvaluationStep **new_steps = realloc(steps->steps, sizeof(EvaluationStep *) * new_capacity);
        if (!new_steps)
        {
            fprintf(stderr, "Memory reallocation failed in add_evaluation_step\n");
            return;
        }
        steps->steps = new_steps;
        steps->capacity = new_capacity;
    }

    EvaluationStep *step = malloc(sizeof(EvaluationStep));
    if (!step)
    {
        fprintf(stderr, "Memory allocation failed for evaluation step\n");
        return;
    }

    step->step_description = strdup(description);
    if (!step->step_description)
    {
        fprintf(stderr, "Memory allocation failed for step description\n");
        free(step);
        return;
    }

    steps->steps[steps->step_count++] = step;
}

void free_evaluation_steps(EvaluationSteps *steps)
{
    if (!steps)
    {
        return;
    }

    for (int i = 0; i < steps->step_count; i++)
    {
        if (steps->steps[i])
        {
            free(steps->steps[i]->step_description);
            free(steps->steps[i]);
        }
    }
    free(steps->steps);
    free(steps);
}

// Node type to string
static const char *get_node_type_str(NodeType type)
{
    switch (type)
    {
    case NODE_VAR:
        return "Variable";
    case NODE_ASSIGN:
        return "Assignment";
    case NODE_NOT:
        return "NOT";
    case NODE_AND:
        return "AND";
    case NODE_OR:
        return "OR";
    case NODE_XOR:
        return "XOR";
    case NODE_XNOR:
        return "XNOR";
    case NODE_IMPLIES:
        return "IMPLIES";
    case NODE_IFF:
        return "IFF";
    case NODE_EQUIV:
        return "EQUIV";
    case NODE_EXISTS:
        return "EXISTS";
    case NODE_FORALL:
        return "FORALL";
    case NODE_BOOL:
        return "Boolean";
    default:
        return "Unknown";
    }
}

// Parse string wrapper
static Node *parse_string(const char *input)
{
    if (!input)
    {
        return NULL;
    }

    yydebug = 1; // Set to 1 for debug output
    yy_scan_string(input);
    parsed_expression = NULL;
    int parse_result = yyparse();

    if (parse_result != 0)
    {
        fprintf(stderr, "Parse error for input: %s\n", input);
        return NULL;
    }

    return parsed_expression;
}

// New function to handle multiple expressions from a single file - FIXED
EvaluationSteps *evaluate_multiple_expressions(const char *expressions)
{
    if (!expressions)
        return NULL;

    EvaluationSteps *steps = init_evaluation_steps();
    if (!steps)
    {
        return NULL;
    }

    SymbolTable *symbol_table = init_symbol_table();
    if (!symbol_table)
    {
        free_evaluation_steps(steps);
        return NULL;
    }

    add_evaluation_step(steps, "Starting evaluation of multiple expressions");

    // Split input by newlines and evaluate each line
    char *input_copy = strdup(expressions);
    if (!input_copy)
    {
        free_evaluation_steps(steps);
        free_symbol_table(symbol_table);
        return NULL;
    }

    char *line = strtok(input_copy, "\n");
    while (line != NULL)
    {
        // Skip empty lines
        if (strlen(line) > 0)
        {
            char desc[256];
            snprintf(desc, sizeof(desc), "Evaluating expression: %s", line);
            add_evaluation_step(steps, desc);

            // Parse the expression
            Node *ast = parse_string(line);
            if (!ast)
            {
                add_evaluation_step(steps, "Failed to parse expression");
            }
            else
            {
                // Evaluate the expression with symbol table
                Node *result = evaluate_node_with_symbol_table(ast, symbol_table, steps);

                // Record result
                if (result && result->type == NODE_BOOL)
                {
                    snprintf(desc, sizeof(desc), "Result: %s", result->bool_val ? "TRUE" : "FALSE");
                    add_evaluation_step(steps, desc);

                    // Free the result
                    free_ast(result);
                }
                else
                {
                    add_evaluation_step(steps, "Could not determine a boolean result for this expression");

                    // Free result if it exists and is not ast (which would already be freed)
                    if (result)
                    {
                        free_ast(result);
                    }
                }
            }
        }
        line = strtok(NULL, "\n");
    }

    add_evaluation_step(steps, "Completed evaluation of all expressions");

    free(input_copy);
    free_symbol_table(symbol_table);
    return steps;
}

// Single expression evaluation (wrapper for backward compatibility)
EvaluationSteps *evaluate_expression(const char *expression)
{
    return evaluate_multiple_expressions(expression);
}

// FFI code
int get_steps_count(EvaluationSteps *steps)
{
    return steps ? steps->step_count : 0;
}

char *get_step_at(EvaluationSteps *steps, int index)
{
    if (!steps || index < 0 || index >= steps->step_count)
        return NULL;
    return steps->steps[index]->step_description;
}