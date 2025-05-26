%{
#include "ast.h"
extern int yylex();
extern int yyparse();
void yyerror(const char *s);

// Declare the global variable as extern (defined in parser_globals.c)
extern Node* parsed_expression;
%}

%union {
    char* str;
    struct Node* node;
    int bool_val;
}

%token <str> IDENTIFIER
%token <bool_val> T_TRUE T_FALSE
%token INVALID_TOKEN


%type <node> expr statement program

%token AND OR NOT XOR XNOR
%token IMPLIES IFF EQUIV
%token EXISTS FORALL
%token IF IFF_KEYWORD
%token ASSIGN LPAREN RPAREN

/* Operator precedence and associativity (lowest to highest) */
%left IFF EQUIV                /* lowest precedence */
%right IMPLIES
%left XOR XNOR
%left OR
%left AND
%right NOT EXISTS FORALL       /* highest precedence */

%%

program:
    statement              { parsed_expression = $1; }
    | program statement    { parsed_expression = $2; }
    ;

statement:
    IDENTIFIER ASSIGN expr        { $$ = create_assignment_node($1, $3); }
    | expr                        { $$ = $1; }
    ;

expr:
      IDENTIFIER                    { $$ = create_variable_node($1); }
    | T_TRUE                        { $$ = create_boolean_node($1); }
    | T_FALSE                       { $$ = create_boolean_node($1); }
    | NOT expr                      { $$ = create_not_node($2); }
    | expr AND expr                 { $$ = create_and_node($1, $3); }
    | expr OR expr                  { $$ = create_or_node($1, $3); }
    | expr XOR expr                 { $$ = create_xor_node($1, $3); }
    | expr XNOR expr                { $$ = create_xnor_node($1, $3); }
    | expr IMPLIES expr             { $$ = create_implies_node($1, $3); }
    | expr IFF expr                 { $$ = create_iff_node($1, $3); }
    | expr EQUIV expr               { $$ = create_equiv_node($1, $3); }
    | EXISTS IDENTIFIER LPAREN expr RPAREN        { $$ = create_exists_node($2, $4); }
    | FORALL IDENTIFIER LPAREN expr RPAREN        { $$ = create_forall_node($2, $4); }
    | LPAREN expr RPAREN            { 
        // Set the is_parenthesized flag for the expression
        $2->is_parenthesized = 1; 
        $$ = $2; 
      }
    | error { yyerror("Syntax error: invalid expression"); YYABORT; }
    ;

%%
