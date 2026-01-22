%{
	#include <stdio.h>
	#include "y.tab.h"
%}
%%
[1-9][0-9]*	{
             //printf("%s",yytext);
						 yylval.ival = atoi(yytext);
						 return NUM;
						}
[ \t\n]
.						{ return(yytext[0]); }
%%
int yywrap(){
	return 1;
}
