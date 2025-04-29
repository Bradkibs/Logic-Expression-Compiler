%%
"AND"|"and"|"&&"                     return AND;
"OR"|"or"|"||"                       return OR;
"NOT"|"not"|"~"                      return NOT;
"XOR"|"xor"                          return XOR;
"XNOR"|"xnor"                        return XNOR;

"->"|"-->"|"==>"|"IMPLIES"|"implies" return IMPLIES;
"<->"|"<=>"|"<==>"|"DOUBLEIMPLIES"|"D_IMPLIES" return IFF;

"="|"EQUALS"|"equals"                                 return ASSIGN;
"==="|"EQUIVALENT"|"equivalent"      return EQUIV;

"E_Q"                                return EXISTS;
"U_Q"                                return FORALL;

"IF"|"if"                            return IF;
"IFF"|"iff"                          return IFF_KEYWORD;

"("                                  return LPAREN;
")"                                  return RPAREN;

[A-Za-z_][A-Za-z0-9_]*               { yylval.str = strdup(yytext); return IDENTIFIER; }

[ \t\n\r]+                           ;  // skips whitespace

.                                    { printf("Unknown character detected: %s\n", yytext); }
%%
