%{
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>
%}

%%

"extern"               { return EXTERN; }
"void"                 { return VOID; }
"int"                  { return INT; }
"if"                   { return IF; }
"else"                 { return ELSE; }
"while"                { return WHILE; }
"return"               { return RETURN; }
"print"                { return PRINT; }
"read"                 { return READ; }
"=="                   { return EQ; }
"!="                   { return NEQ; }
"<="                   { return LE; }
">="                   { return GE; }
[0-9]+                 { yylval.num = atoi(yytext); return NUM; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str = strdup(yytext); return NAME; }
[ \t\n]+               { /* skip whitespace */ }
.                      { return yytext[0]; }

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
