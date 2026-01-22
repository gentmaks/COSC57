%{
#include "y.tab.h"
#include <string.h>
#include <stdlib.h>
%}

%%

[A-Z][a-zA-Z]* { yylval.str = strdup(yytext); return NAME; }
":"            { return COLON; }
[0-9]+         { yylval.num = atoi(yytext); return NUMBER; }
","            { ; }
[ \t]          { ; }
\n             { return NEWLINE; }
.              { return yytext[0]; }


%%

int yywrap() {
    return 1;
}
