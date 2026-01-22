%{
#include <stdlib.h>
#include <stdio.h>
void yyerror(char *s);
int yylex();
%}

%union {
    int num;
    char *str;
}

%token <num> NUM
%token <str> NAME
%start statement
%%

statement: 
         /*   */
         | statement term
         ;
term:
    NUM {printf("num: %d\n", $1); }
    | NAME {printf("name: %s\n", $1); }
    ;

%%

void yyerror(char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

int main() {
    printf("Processing minic program \n\n");
    yyparse();
    return 0;
}
