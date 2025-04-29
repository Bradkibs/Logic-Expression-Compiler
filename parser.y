%{
#include "ast.h"
extern int yylex();
extern int yyparse();
void yyerror(const char *s);
%}

%union {
    char* str;
    struct Node* node;
}

%token <str> IDENTIFIER
%type <node> expr statement program

%token AND OR NOT XOR XNOR
%token IMPLIES IFF EQUIV
%token EXISTS FORALL
%token IF IFF_KEYWORD
%token ASSIGN LPAREN RPAREN

%%

program:
    | program statement
    ;

statement:
    IDENTIFIER ASSIGN expr        { $$ = create_assignment_node($1, $3); }
    | expr                        { $$ = $1; }
    ;

expr:
      IDENTIFIER                    { $$ = create_variable_node($1); }
    | NOT expr                      { $$ = create_not_node($2); }
    | expr AND expr                 { $$ = create_and_node($1, $3); }
    | expr OR expr                  { $$ = create_or_node($1, $3); }
    | expr XOR expr                 { $$ = create_xor_node($1, $3); }
    | expr XNOR expr                { $$ = create_xnor_node($1, $3); }
    | expr IMPLIES expr             { $$ = create_implies_node($1, $3); }
    | expr IFF expr                 { $$ = create_iff_node($1, $3); }
    | expr EQUIV expr               { $$ = create_equiv_node($1, $3); }
    | EXISTS IDENTIFIER expr        { $$ = create_exists_node($2, $3); }
    | FORALL IDENTIFIER expr        { $$ = create_forall_node($2, $3); }
    | LPAREN expr RPAREN            { $$ = $2; }
    ;
