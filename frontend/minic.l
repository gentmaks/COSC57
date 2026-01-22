%{
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>
%}

%%

[0-9]+    { yylval.num = atoi(yytext); return NUM; }
[a-zA-Z]+ { yylval.str = strdup(yytext); return NAME; }
[ \t\n]   { ; }
.         {return yytext[0]; }


%%

int yywrap() {
    return 1;
}

// "{"       { return LBRACE; }
// "}"       { return RBRACE; }
// ";"       { return SEMICOLON; }
// "+"       { return PLUS; }
// "-"       { return MINUS; }
// "*"       { return MULT; }
// "/"       { return DIV; }
// "\n"      { return NEWLINE; }
