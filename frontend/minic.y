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
%token PRINT RETURN
%token EXTERN VOID INT IF ELSE WHILE READ
%token EQ NEQ LE GE

%left '+' '-'
%left '*' '/'
%start statement_list
%%

statement_list:
    statement_list statement
        { printf("stmt_list: append statement\n"); }
    | statement
        { printf("stmt_list: single statement\n"); }
    ;

statement: 
    NAME '=' expr ';'
        { printf("statement: assignment to %s\n", $1); }
    | PRINT '(' expr ')' ';'
        { printf("statement: print\n"); }
    | RETURN expr ';'
        { printf("statement: return\n"); }
    | RETURN ';'
        {printf("Emptry return\n"); }
    ;

expr: expr '+' expr
        { printf("expr: add\n"); }
    | expr '-' expr
        { printf("expr: sub\n"); }
    | expr '*' expr
        { printf("expr: mul\n"); }
    | expr '/' expr
        { printf("expr: div\n"); }
    | '(' expr ')'
        { printf("expr: paren\n"); }
    | NUM
        { printf("expr: num %d\n", $1); }
    | NAME
        { printf("expr: name %s\n", $1); }

%%

void yyerror(char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

int main() {
    printf("Processing minic program \n\n");
    yyparse();
    return 0;
}
