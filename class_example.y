%{
#include<stdio.h>
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
int yyerror(char *);
extern FILE *yyin;
%}
%union{
	int ival;
	char *sname;
}

%token <ival> NUM 
%token <sname> NAME
%type <ival> score scores
%start students
%%
students : students student
				 | student
student	 : NAME ':' scores {
														 printf("Total score of %s is %d\n", $1, $3);
														 free($1);
													 }
scores	 : scores score {$$ = $1 + $2;}
				 | score {$$ = $1;}
score		 : NUM {$$ = $1;}

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
		
		//cleanup
		if (argc == 2) fclose(yyin);
		yylex_destroy();
		return 0;
}
