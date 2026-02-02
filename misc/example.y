%{
#include<stdio.h>
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
int yyerror(char*);
extern FILE *yyin;
%}
%token NUM
%start expr
%%
expr : expr '+'  term
		 | expr '-'	 term
		 | term
term : NUM
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
		if (argc == 2) fclose(yyin);
		yylex_destroy();
		return 0;
}
