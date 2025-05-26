%{
#include <stdio.h>
#include <string.h>
#include "parser.h"

extern YYSTYPE yylval;
%}

%option noyywrap
%option yylineno
%option noinput nounput



%%
"AND"|"and"|"&&"                     return AND;
"OR"|"or"|"||"                       return OR;
"NOT"|"not"|"~"                      return NOT;
"XOR"|"xor"                          return XOR;
"XNOR"|"xnor"                        return XNOR;

"->"|"-->"|"==>"|"=>"|"IMPLIES"|"implies" return IMPLIES;
"<->"|"<=>"|"<-->"|"<==>"|"DOUBLEIMPLIES"|"D_IMPLIES" return IFF;

"="|"EQUALS"|"equals"                                 return ASSIGN;
"==="|"EQUIVALENT"|"equivalent"      return EQUIV;

"E_Q"                                return EXISTS;
"U_Q"                                return FORALL;

"IF"|"if"                            return IF;
"IFF"|"iff"                          return IFF_KEYWORD;

"("                                  return LPAREN;
")"                                  return RPAREN;

"true"|"TRUE"    { yylval.bool_val = 1; return T_TRUE; }
"false"|"FALSE"  { yylval.bool_val = 0; return T_FALSE; }


[A-Za-z_][A-Za-z0-9_]*               { yylval.str = strdup(yytext); return IDENTIFIER; }

[ \t\n\r]+                           ;  // skips whitespace

.   {
    fprintf(stderr, "Lexer error: Unrecognized character '%s' at line %d\n", yytext, yylineno);
    return INVALID_TOKEN;
}

%%
