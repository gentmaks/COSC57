%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(char *s);
int yylex();
%}

%union {
int num;
char *str;
}

%token <str> NAME
%token <num> NUMBER
%token COLON NEWLINE
%type <num> score_list

%%

input:
    /* empty - this allows the recursion to start */
    | input line
    ;

line:
    NAME COLON score_list NEWLINE {
        printf("%s: %d\n", $1, $3);
        free($1);
    }
    | NEWLINE
    ;

score_list:
      NUMBER {
        $$ = $1;
      }
      | score_list NUMBER {
        $$ = $1 + $2;
      }
      ;

%%

void yyerror(char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

int main() {
    printf("Processing student scores ... \n\n");
    yyparse();
    return 0;
}
