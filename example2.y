%{
#include<stdio.h>
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
int yyerror(char *);
extern FILE *yyin;
int value;
%}
%union{
	int ival;
}

%token <ival> NUM 
%type <ival> expr term
%start start
%%
start 	 : exprs
exprs 	 : exprs expr {printf("Expression value: %d\n", $2);}
				 | expr {printf("Expression value: %d\n", $1);}
expr		 : term '+' expr {$$ = $1 + $3;}
				 | term '-'	expr {$$ = $1 - $3;}
				 | term {$$ = $1;}
term		 : NUM {$$ = $1;}

%%
int yyerror(char *s){
	fprintf(stderr,"%s\n", s);
	return 0;
}

int main(int argc, char* argv[]){
		if (argc == 2){
			yyin = fopen(argv[1], "r");
			if (yyin == NULL) {
				fprintf(stderr, "File open error\n");
				return 1;
			}
		}
		yyparse();
		
		// Cleanup
		if (argc == 2) fclose(yyin);
		yylex_destroy();
		return 0;
}
