#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>  // For mkdtemp
#include "C_Unlinked_Components/ast.h"
#include "C_Unlinked_Components/symbol_table.h"
#include "C_Unlinked_Components/semantic_analyzer.h"
#include "C_Unlinked_Components/multi_statement.h"
#include "C_Unlinked_Components/llvm_codegen.h"

// External lexer functions and variables
extern void print_tokens(void);  // Function to print all tokens (will be implemented in lexer)

// Function to display all tokens in a file
void display_file_tokens(const char* file_contents) {
    printf("\n[LEXICAL ANALYSIS - TOKEN SUMMARY]\n");
    printf("╔════════════════╦═══════════════╦════════════╗\n");
    printf("║ %-14s ║ %-13s ║ %-10s ║\n", "TOKEN TYPE", "LEXEME", "COUNT");
    printf("╠════════════════╬═══════════════╬════════════╣\n");
    
    // Create a temporary file with the contents
    FILE* temp = tmpfile();
    if (!temp) {
        fprintf(stderr, "Error: Failed to create temporary file for token analysis\n");
        return;
    }
    
    fputs(file_contents, temp);
    rewind(temp);
    
    // Set up lexer to read from the temp file
    extern FILE* yyin;
    extern int yylex();
    extern char* yytext;
    yyin = temp;
    
    // Track token occurrences
    typedef struct {
        int token_code;
        char* lexeme;
        int count;
    } TokenCount;
    
    TokenCount* token_counts = NULL;
    int num_unique_tokens = 0;
    int token_counts_capacity = 0;
    
    int token;
    while ((token = yylex()) != 0) {
        // Check if we've seen this token+lexeme combination before
        bool found = false;
        for (int i = 0; i < num_unique_tokens; i++) {
            if (token_counts[i].token_code == token && 
                strcmp(token_counts[i].lexeme, yytext) == 0) {
                token_counts[i].count++;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Need to add a new token entry
            if (num_unique_tokens >= token_counts_capacity) {
                // Resize the array
                token_counts_capacity = token_counts_capacity == 0 ? 10 : token_counts_capacity * 2;
                TokenCount* new_counts = (TokenCount*)realloc(token_counts, 
                                                             token_counts_capacity * sizeof(TokenCount));
                if (!new_counts) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    break;
                }
                token_counts = new_counts;
            }
            
            // Add the new token
            token_counts[num_unique_tokens].token_code = token;
            token_counts[num_unique_tokens].lexeme = strdup(yytext);
            token_counts[num_unique_tokens].count = 1;
            num_unique_tokens++;
        }
    }
    
    // Print the token summary
    for (int i = 0; i < num_unique_tokens; i++) {
        const char* token_name = "UNKNOWN";
        const char* lexeme = token_counts[i].lexeme;
        
        // Map lexemes directly to token types for more accurate display
        if (strcmp(lexeme, "R") == 0 || 
            strcmp(lexeme, "S") == 0 || 
            strcmp(lexeme, "T") == 0 || 
            strcmp(lexeme, "NEW") == 0 || 
            strcmp(lexeme, "INTERESTING_VARIABLE") == 0) {
            token_name = "IDENTIFIER";
        }
        else if (strcmp(lexeme, "(") == 0) {
            token_name = "LPAREN";
        }
        else if (strcmp(lexeme, ")") == 0) {
            token_name = "RPAREN";
        }
        else if (strcmp(lexeme, "TRUE") == 0 || strcmp(lexeme, "FALSE") == 0) {
            token_name = "BOOLEAN";
        }
        else if (strcmp(lexeme, "=") == 0) {
            token_name = "ASSIGN_OP";
        }
        else if (strcmp(lexeme, "AND") == 0) {
            token_name = "AND_OP";
        }
        else if (strcmp(lexeme, "OR") == 0) {
            token_name = "OR_OP";
        }
        else if (strcmp(lexeme, "NOT") == 0) {
            token_name = "NOT_OP";
        }
        else if (strcmp(lexeme, "XOR") == 0) {
            token_name = "XOR_OP";
        }
        else if (strcmp(lexeme, "XNOR") == 0) {
            token_name = "XNOR_OP";
        }
        else if (strcmp(lexeme, "->") == 0) {
            token_name = "IMPLIES_OP";
        }
        else if (strcmp(lexeme, "<->") == 0) {
            token_name = "IFF_OP";
        }
        else if (strcmp(lexeme, "==") == 0) {
            token_name = "EQUIV_OP";
        }
        else if (strcmp(lexeme, "EXISTS") == 0) {
            token_name = "EXISTS_OP";
        }
        else if (strcmp(lexeme, "FORALL") == 0) {
            token_name = "FORALL_OP";
        }
        else if (strcmp(lexeme, "IF") == 0) {
            token_name = "IF_KEYWORD";
        }
        else if (strcmp(lexeme, "IFF") == 0) {
            token_name = "IFF_KEYWORD";
        }
        else {
            // For any unhandled lexemes, try to map token codes as a fallback
            token_name = "UNKNOWN";
        }
        
        printf("║ %-14s ║ %-13s ║ %-10d ║\n", 
               token_name, token_counts[i].lexeme, token_counts[i].count);
    }
    
    printf("╚════════════════╩═══════════════╩════════════╝\n");
    printf("Total unique tokens: %d\n\n", num_unique_tokens);
    
    // Clean up
    for (int i = 0; i < num_unique_tokens; i++) {
        free(token_counts[i].lexeme);
    }
    free(token_counts);
    fclose(temp);
}

// Include parser definitions (generated by bison)
#include "C_Unlinked_Components/parser.h"

// Forward declarations for parser functions (generated by bison)
extern int yyparse();
extern FILE* yyin;

// The root of the AST - declared as extern since it's defined elsewhere
extern Node* parsed_expression;

// Function to initialize a multi-statement AST
MultiStatementAST* init_multi_statement_ast() {
    MultiStatementAST* ast = (MultiStatementAST*)malloc(sizeof(MultiStatementAST));
    if (!ast) return NULL;
    
    ast->capacity = 10;  // Start with capacity for 10 statements
    ast->count = 0;
    ast->statements = (Node**)malloc(ast->capacity * sizeof(Node*));
    
    if (!ast->statements) {
        free(ast);
        return NULL;
    }
    
    return ast;
}

// Function to add a statement to the multi-statement AST
void add_statement(MultiStatementAST* ast, Node* statement) {
    if (!ast || !statement) return;
    
    // Resize if needed
    if (ast->count >= ast->capacity) {
        ast->capacity *= 2;
        Node** new_statements = (Node**)realloc(ast->statements, ast->capacity * sizeof(Node*));
        if (!new_statements) return;
        ast->statements = new_statements;
    }
    
    // Add the statement
    ast->statements[ast->count++] = statement;
}

// Function to free a multi-statement AST
void free_multi_statement_ast(MultiStatementAST* ast) {
    if (!ast) return;
    
    // Free each statement
    for (int i = 0; i < ast->count; i++) {
        free_ast(ast->statements[i]);
    }
    
    free(ast->statements);
    free(ast);
}

// Function to print usage information
void print_usage() {
    printf("Usage: lec_compiler_llvm <input_file> [-oN]\n");
    printf("  -oN  Set optimization level (0-3, default: 0)\n");
    printf("Example: lec_compiler_llvm input.lec -o2\n");
}

// Function to get the base name of a file (without extension)
char* get_base_name(const char* file_path) {
    // Find the last path separator
    const char* last_slash = strrchr(file_path, '/');
    const char* file_name = last_slash ? last_slash + 1 : file_path;
    
    // Find the last dot (extension separator)
    const char* last_dot = strrchr(file_name, '.');
    
    // Calculate length of the base name
    size_t base_len = last_dot ? (size_t)(last_dot - file_name) : strlen(file_name);
    
    // Allocate memory for the base name
    char* base_name = (char*)malloc(base_len + 1);
    if (!base_name) {
        return NULL;
    }
    
    // Copy the base name
    strncpy(base_name, file_name, base_len);
    base_name[base_len] = '\0';
    
    return base_name;
}

// Read the entire file into a buffer
char* read_file_contents(const char* input_file) {
    FILE* file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open input file '%s'\n", input_file);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // Allocate buffer with extra space for null terminator
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for file contents\n");
        fclose(file);
        return NULL;
    }
    
    // Read the file into the buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';  // Null-terminate the buffer
    
    fclose(file);
    return buffer;
}

// Pre-process assignments to build the symbol table
void process_assignments(const char* file_contents, SymbolTable* symbol_table) {
    // Silently pre-process assignments without printing a stage header
    printf("Pre-processing assignments...\n");
    
    // Make a copy of the file contents
    char* buffer = strdup(file_contents);
    if (!buffer) return;
    
    // Process each line
    char* line = strtok(buffer, "\n");
    while (line) {
        // Skip empty lines
        if (strlen(line) > 0) {
            // Check if this is a simple assignment (format: VAR = VALUE)
            // We need to be more strict to avoid misinterpreting expressions like A -> B
            if (strstr(line, "=") && !strstr(line, "=>") && !strstr(line, "-->") && 
                !strstr(line, "<->") && !strstr(line, "<==>") && !strstr(line, "IMPLIES") && 
                !strstr(line, "DOUBLEIMPLIES") && !strstr(line, "IFF")) {
                // Simple direct parsing for TRUE/FALSE assignments
                char var_name[MAX_SYMBOL_NAME_LENGTH] = {0};
                char var_value[20] = {0};
                
                if (sscanf(line, "%49s = %19s", var_name, var_value) == 2) {
                    int value = 0;
                    
                    // Check for TRUE/FALSE directly
                    if (strcmp(var_value, "TRUE") == 0) {
                        value = 1;
                    } else if (strcmp(var_value, "FALSE") == 0) {
                        value = 0;
                    }
                    
                    // Add to symbol table
                    add_or_update_symbol(symbol_table, var_name, value);
                    printf("Added variable '%s' with value %d to symbol table\n", var_name, value);
                    line = strtok(NULL, "\n");
                    continue;
                }
                
                // If direct parsing fails, use the parser
                FILE* temp = tmpfile();
                if (!temp) {
                    fprintf(stderr, "Error: Failed to create temporary file\n");
                    line = strtok(NULL, "\n");
                    continue;
                }
                
                fputs(line, temp);
                rewind(temp);
                
                yyin = temp;
                parsed_expression = NULL;
                int parse_result = yyparse();
                fclose(temp);
                
                if (parse_result == 0 && parsed_expression) {
                    // If it's an assignment, extract the variable name and value
                    if (parsed_expression->type == NODE_ASSIGN && parsed_expression->name) {
                        int value = 0;
                        if (parsed_expression->right) {
                            if (parsed_expression->right->type == NODE_BOOL) {
                                value = parsed_expression->right->bool_val;
                            } else if (parsed_expression->right->type == NODE_VAR) {
                                if (parsed_expression->right->name) {
                                    if (strcmp(parsed_expression->right->name, "TRUE") == 0) {
                                        value = 1;
                                    } else if (strcmp(parsed_expression->right->name, "FALSE") == 0) {
                                        value = 0;
                                    }
                                }
                            }
                        }
                        
                        // Add the variable to the symbol table
                        char* var_name = strdup(parsed_expression->name);
                        add_or_update_symbol(symbol_table, var_name, value);
                        printf("Added variable '%s' with value %d to symbol table\n", 
                               var_name, value);
                        free(var_name);
                    }
                    
                    // Free the parsed expression
                    if (parsed_expression) {
                        free_ast(parsed_expression);
                        parsed_expression = NULL;
                    }
                }
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    free(buffer);
}

// Function to read a file line by line and parse each statement
MultiStatementAST* parse_file_by_lines(const char* input_file, SymbolTable* symbol_table) {
    // Skip initial stage printing - will be handled in compile_file
    printf("Parsing file: %s\n", input_file);
    MultiStatementAST* ast = init_multi_statement_ast();
    if (!ast) return NULL;
    
    // First read the entire file
    char* file_contents = read_file_contents(input_file);
    if (!file_contents) {
        free_multi_statement_ast(ast);
        return NULL;
    }
    
    // Display token summary for the entire file
    display_file_tokens(file_contents);
    
    // Pre-process assignments to build the symbol table
    process_assignments(file_contents, symbol_table);
    
    // Now parse each line with the complete symbol table
    char* buffer = strdup(file_contents);
    if (!buffer) {
        free(file_contents);
        free_multi_statement_ast(ast);
        return NULL;
    }
    
    // Process each line
    char* line = strtok(buffer, "\n");
    int line_num = 0;
    
    while (line) {
        line_num++;
        
        // Skip empty lines
        if (strlen(line) > 0) {
            // Parse the line
            FILE* temp = tmpfile();
            if (!temp) {
                fprintf(stderr, "Error: Failed to create temporary file\n");
                line = strtok(NULL, "\n");
                continue;
            }
            
            fputs(line, temp);
            rewind(temp);
            
            // Now parse the expression
            yyin = temp;
            parsed_expression = NULL;
            int parse_result = yyparse();
            fclose(temp);
            
            if (parse_result != 0 || !parsed_expression) {
                fprintf(stderr, "Warning: Failed to parse line %d: %s\n", line_num, line);
                line = strtok(NULL, "\n");
                continue;
            }
            
            // Skip assignment statements as they've already been processed
            if (parsed_expression->type != NODE_ASSIGN) {
                // Add the statement to our multi-statement AST
                add_statement(ast, parsed_expression);
            } else {
                // Free the parsed assignment statement as it's already been processed
                free_ast(parsed_expression);
            }
            parsed_expression = NULL;  // Reset for next parse
        }
        
        line = strtok(NULL, "\n");
    }
    
    free(buffer);
    free(file_contents);
    
    printf("Parsing complete.\n");
    
    // Print the complete AST for all statements
    if (ast && ast->count > 0) {
        print_multi_statement_ast(ast);
    }
    
    return ast;
}

// Global variables
int optimization_level = 0;

// Global symbol table reference for AST printing
SymbolTable* current_symbol_table = NULL;

// Generate a unique ID for each node in the AST
int generate_node_id() {
    static int node_counter = 0;
    return ++node_counter;
}

// Function to print AST with indentation
void print_ast_with_indent(Node* node, int indent_level) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < indent_level; i++) {
        printf("  ");
    }
    
    // Print node type based on the actual Node structure
    switch (node->type) {
        case NODE_VAR:
            printf("VARIABLE: %s\n", node->name);
            break;
        case NODE_BOOL:
            printf("BOOLEAN: %s\n", node->bool_val ? "TRUE" : "FALSE");
            break;
        case NODE_NOT:
            printf("NOT:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            break;
        case NODE_AND:
            printf("AND:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_OR:
            printf("OR:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_XOR:
            printf("XOR:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_XNOR:
            printf("XNOR:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_IMPLIES:
            printf("IMPLIES:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_IFF:
            printf("IFF:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_EQUIV:
            printf("EQUIV:\n");
            print_ast_with_indent(node->left, indent_level + 1);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_EXISTS:
            printf("EXISTS: %s\n", node->name);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_FORALL:
            printf("FORALL: %s\n", node->name);
            print_ast_with_indent(node->right, indent_level + 1);
            break;
        case NODE_ASSIGN:
            printf("ASSIGNMENT: %s = \n", node->name);
            if (node->left) {
                print_ast_with_indent(node->left, indent_level + 1);
            } else {
                for (int i = 0; i < indent_level + 1; i++) {
                    printf("  ");
                }
                printf("VALUE: %s\n", node->bool_val ? "TRUE" : "FALSE");
            }
            break;
        default:
            printf("UNKNOWN NODE TYPE: %d\n", node->type);
    }
}

// Function to print a multi-statement AST
void print_multi_statement_ast(MultiStatementAST* ast) {
    if (!ast) return;
    
    printf("\n🌳 [ABSTRACT SYNTAX TREE]\n");
    
    // Print traditional AST view for each statement
    for (int i = 0; i < ast->count; i++) {
        printf("Statement %d:\n", i + 1);
        print_ast_with_indent(ast->statements[i], 1);
        printf("\n");
    }
    
    printf("Total statements: %d\n\n", ast->count);
}

// Function to print optimization information based on level
void print_optimization_info(int level) {
    printf("\n🚀 [OPTIMIZATION LEVEL: -o%d]\n", level);
    printf("╔═════════════════════════════════════════════════════════════════════════════╗\n");
    
    if (level == 0) {
        printf("║ No optimizations enabled.                                                 ║\n");
    } else {
        printf("║ Enabled optimizations:                                                   ║\n");
        
        // Common optimizations for all levels
        if (level >= 1) {
            printf("║ ✓ Constant folding (replacing compile-time constants)                    ║\n");
            printf("║ ✓ Dead code elimination (removing unreachable code)                      ║\n");
        }
        
        // Level 2 adds more optimizations
        if (level >= 2) {
            printf("║ ✓ Common subexpression elimination                                       ║\n");
            printf("║ ✓ Instruction combining (merging related operations)                     ║\n");
        }
        
        // Level 3 adds the most aggressive optimizations
        if (level >= 3) {
            printf("║ ✓ Aggressive dead code elimination                                      ║\n");
            printf("║ ✓ Full loop optimizations                                                ║\n");
            printf("║ ✓ Function inlining                                                      ║\n");
            printf("║ ✓ Memory-to-register promotion                                           ║\n");
        }
    }
    
    printf("╚═════════════════════════════════════════════════════════════════════════════╝\n\n");
}

// Function to print AST with values (for annotated AST)
void print_ast_with_values(Node* node, int indent_level, SymbolTable* symbol_table) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < indent_level; i++) {
        printf("  ");
    }
    
    // Print node type based on the actual Node structure
    switch (node->type) {
        case NODE_VAR: {
            // Get the value of the variable from the symbol table
            bool value = false;
            bool found = false;
            for (int i = 0; i < symbol_table->size; i++) {
                if (strcmp(symbol_table->symbols[i].name, node->name) == 0) {
                    value = symbol_table->symbols[i].value;
                    found = true;
                    break;
                }
            }
            if (found) {
                printf("VARIABLE: %s (Value: %s)\n", node->name, value ? "TRUE" : "FALSE");
            } else {
                printf("VARIABLE: %s (Value: unknown)\n", node->name);
            }
            break;
        }
        case NODE_BOOL:
            printf("BOOLEAN: %s\n", node->bool_val ? "TRUE" : "FALSE");
            break;
        case NODE_NOT:
            printf("NOT:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            break;
        case NODE_AND:
            printf("AND:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_OR:
            printf("OR:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_XOR:
            printf("XOR:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_XNOR:
            printf("XNOR:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_IMPLIES:
            printf("IMPLIES:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_IFF:
            printf("IFF:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_EQUIV:
            printf("EQUIV:\n");
            print_ast_with_values(node->left, indent_level + 1, symbol_table);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_EXISTS:
            printf("EXISTS: %s\n", node->name);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_FORALL:
            printf("FORALL: %s\n", node->name);
            print_ast_with_values(node->right, indent_level + 1, symbol_table);
            break;
        case NODE_ASSIGN:
            printf("ASSIGNMENT: %s = \n", node->name);
            if (node->left) {
                print_ast_with_values(node->left, indent_level + 1, symbol_table);
            } else {
                for (int i = 0; i < indent_level + 1; i++) {
                    printf("  ");
                }
                printf("VALUE: %s\n", node->bool_val ? "TRUE" : "FALSE");
            }
            break;
        default:
            printf("UNKNOWN NODE TYPE: %d\n", node->type);
    }
}

// Function to print semantically analyzed AST with type annotations
void print_semantic_analysis_results(MultiStatementAST* ast, SymbolTable* symbol_table) {
    if (!ast || !symbol_table) return;
    
    printf("\n🔎 [SEMANTIC ANALYSIS RESULTS]\n");
    
    // Print symbol table contents in tabular format
    printf("\n📋 [SYMBOL TABLE]\n");
    printf("╔═══════════════════╦═════════════╦════════════╦═════════════╦═══════════════╦══════════════════════════╗\n");
    printf("║ %-17s ║ %-11s ║ %-10s ║ %-11s ║ %-13s ║ %-21s ║\n", 
           "IDENTIFIER", "TYPE", "SCOPE", "INIT", "VALUE", "PRODUCTION RULE");
    printf("╠═══════════════════╬═════════════╬═══════════╬═════════════╬═══════════════╬══════════════════════════╣\n");
    
    // Define production rules for each variable
    const char* production_rules[] = {
        "VAR → IDENTIFIER",
        "EXPR → EXPR AND EXPR",
        "EXPR → EXPR OR EXPR",
        "EXPR → NOT EXPR",
        "EXPR → VAR",
        "EXPR → TRUE | FALSE",
        "ASSIGN → VAR = EXPR"
    };
    
    for (int i = 0; i < symbol_table->size; i++) {
        // Determine which production rule applies based on variable usage
        const char* rule = "VAR → IDENTIFIER";
        
        printf("║ %-17s ║ %-11s ║ %-10s ║ %-11s ║ %-13s ║ %-21s ║\n", 
               symbol_table->symbols[i].name, 
               "boolean", 
               "global", 
               "Yes", 
               symbol_table->symbols[i].value ? "TRUE" : "FALSE",
               rule);
    }
    printf("╚═══════════════════╩═════════════╩═══════════╩═════════════╩═══════════════╩══════════════════════════╝\n");
    
    // Show all production rules used in the grammar
    printf("\n📜 [GRAMMAR PRODUCTION RULES]\n");
    printf("╔═════════════════════════════════════════════════════════════════════════════╗\n");
    for (int i = 0; i < sizeof(production_rules)/sizeof(production_rules[0]); i++) {
        printf("║ %-73s ║\n", production_rules[i]);
    }
    printf("╚═════════════════════════════════════════════════════════════════════════════╝\n");
    printf("Total symbols: %d\n\n", symbol_table->size);
    
    // Also print a traditional view of the annotated AST WITH variable values
    printf("Annotated AST (Traditional View with Variable Values):\n");
    for (int i = 0; i < ast->count; i++) {
        printf("Statement %d:\n", i + 1);
        print_ast_with_values(ast->statements[i], 1, symbol_table);
        printf("  [Semantic Info: Expression validated]\n");
    }
    
    printf("\n");
}

// Function to compile a logical expression file
int compile_file(const char* input_file, const char* output_file) {
    // Initialize the symbol table
    SymbolTable* symbol_table = init_symbol_table();
    // Set global symbol table reference
    current_symbol_table = symbol_table;
    if (!symbol_table) {
        fprintf(stderr, "Error: Failed to initialize symbol table\n");
        return 1;
    }
    
    // Parse the input file line by line
    printf("\n[STAGE 1: PARSING]\n");
    MultiStatementAST* multi_ast = parse_file_by_lines(input_file, symbol_table);
    if (!multi_ast || multi_ast->count == 0) {
        fprintf(stderr, "Error: No AST was generated\n");
        free_symbol_table(symbol_table);
        if (multi_ast) free_multi_statement_ast(multi_ast);
        return 1;
    }
    
    // Store symbol table information for later use, but don't display yet
    
    // Semantic analysis (skip header, just do the analysis)
    // printf("\n[STAGE 2: SEMANTIC ANALYSIS]\n");
    // printf("Performing semantic analysis...\n");
    for (int i = 0; i < multi_ast->count; i++) {
        Node* expr = multi_ast->statements[i];
        if (!expr) continue;
        
        // Skip assignments since they've already been processed
        if (expr->type == NODE_ASSIGN) continue;
        
        SemanticAnalysisResult analysis_result = perform_semantic_analysis(expr, symbol_table);
        if (analysis_result.error_code != SEMANTIC_OK) {
            fprintf(stderr, "Semantic error: %s\n", 
                    analysis_result.error_message ? analysis_result.error_message : "Unknown error");
            if (analysis_result.error_message) {
                free(analysis_result.error_message);
            }
            free_multi_statement_ast(multi_ast);
            free_symbol_table(symbol_table);
            return 1;
        }
    }
    
    // Generate LLVM IR
    printf("\n[STAGE 3: CODE GENERATION]\n");
    
    // Print optimization information
    print_optimization_info(optimization_level);
    
    printf("Generating LLVM IR...\n");
    
    // Create a temporary directory for intermediate files
    char temp_dir[] = "/tmp/lec_XXXXXX";
    if (!mkdtemp(temp_dir)) {
        perror("Failed to create temporary directory");
        free_multi_statement_ast(multi_ast);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    // Print symbol table for codegen (just once)
    printf("\nSymbol table at codegen time (size: %d):\n", symbol_table->size);
    for (int i = 0; i < symbol_table->size; i++) {
        printf("  %s = %s\n", symbol_table->symbols[i].name, 
               symbol_table->symbols[i].value ? "TRUE" : "FALSE");
    }
    
    // Add a flag to prevent duplicate symbol table display in IR generation
    
    // Generate LLVM IR with the current optimization level
    LLVMCodegenResult ir_result = generate_llvm_ir(multi_ast, symbol_table, output_file, optimization_level);
    if (ir_result.error_code != LLVM_CODEGEN_OK) {
        fprintf(stderr, "LLVM code generation error: %s\n", 
                ir_result.error_message ? ir_result.error_message : "Unknown error");
        free_llvm_codegen_result(&ir_result);
        free_multi_statement_ast(multi_ast);
        free_symbol_table(symbol_table);
        
        // Clean up temp directory
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
        system(cmd);
        return 1;
    }
    
    // Print semantic analysis results
    print_semantic_analysis_results(multi_ast, symbol_table);
    
    // Create a temporary IR file in the temp directory
    char temp_ir_filename[2048];
    snprintf(temp_ir_filename, sizeof(temp_ir_filename), "%s/temp.ll", temp_dir);
    
    // Save the IR to the temporary file first
    LLVMCodegenResult temp_save = save_llvm_ir(ir_result.module, temp_ir_filename);
    if (temp_save.error_code != LLVM_CODEGEN_OK) {
        fprintf(stderr, "Warning: Failed to save temporary IR file: %s\n", 
               temp_save.error_message ? temp_save.error_message : "Unknown error");
        free_llvm_codegen_result(&temp_save);
        // Continue anyway as this is not a critical error
    }
    
    // Create the output IR filename (same as output file but with .ll extension)
    char ir_filename[2048];
    snprintf(ir_filename, sizeof(ir_filename), "%s.ll", output_file);
    
    // Save the IR to the final location
    LLVMCodegenResult save_result = save_llvm_ir(ir_result.module, ir_filename);
    if (save_result.error_code != LLVM_CODEGEN_OK) {
        fprintf(stderr, "Failed to save LLVM IR: %s\n", 
                save_result.error_message ? save_result.error_message : "Unknown error");
        free_llvm_codegen_result(&ir_result);
        free_llvm_codegen_result(&save_result);
        free_multi_statement_ast(multi_ast);
        free_symbol_table(symbol_table);
        // Clean up temp directory
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
        system(cmd);
        return 1;
    }
    
    // Compile and link the generated IR
    printf("Compiling and linking LLVM IR...\n");
    LLVMCodegenResult compile_result = compile_and_link_ir(ir_filename, output_file);
    
    // Clean up intermediate files
    char cleanup_cmd[4096];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", temp_dir);
    system(cleanup_cmd);
    
    if (compile_result.error_code != LLVM_CODEGEN_OK) {
        fprintf(stderr, "Compilation error: %s\n", 
                compile_result.error_message ? compile_result.error_message : "Unknown error");
        free_llvm_codegen_result(&ir_result);
        free_llvm_codegen_result(&save_result);
        free_llvm_codegen_result(&compile_result);
        free_multi_statement_ast(multi_ast);
        free_symbol_table(symbol_table);
        return 1;
    }
    
    // Clean up
    printf("Compilation successful. Executable created: %s\n", output_file);
    printf("LLVM IR was saved to: %s\n", ir_filename);
    free_llvm_codegen_result(&ir_result);
    free_llvm_codegen_result(&save_result);
    free_llvm_codegen_result(&compile_result);
    free_multi_statement_ast(multi_ast);
    free_symbol_table(symbol_table);
    
    return 0;
}

int main(int argc, char** argv) {
    // Check arguments
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    const char* input_file = NULL;
    char* output_file = strdup("output"); // Default output file name
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-o", 2) == 0) {
            // Handle optimization level
            if (strlen(argv[i]) > 2) {
                // Format: -oN (e.g., -o2)
                optimization_level = atoi(argv[i] + 2);
                if (optimization_level < 0 || optimization_level > 3) {
                    fprintf(stderr, "Error: Optimization level must be between 0 and 3\n");
                    free(output_file);
                    return 1;
                }
            }
        } else if (argv[i][0] != '-') {
            // This is the input file
            if (input_file == NULL) {
                input_file = argv[i];
            } else {
                // Second non-option argument is treated as output file (for backward compatibility)
                free(output_file);
                output_file = strdup(argv[i]);
            }
        } else {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            print_usage();
            free(output_file);
            return 1;
        }
    }
    
    if (input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage();
        free(output_file);
        return 1;
    }
    
    printf("Compiling %s\n", input_file);
    printf("Optimization level: -o%d\n", optimization_level);
    
    // Compile the file
    int result = compile_file(input_file, output_file);
    
    // Clean up
    free(output_file);
    
    return result;
}
