%{
#include <stdlib.h>
#include <stdio.h>
#include "ast.h"
void yyerror(char *s);
int yylex();
astNode *rootNode;
%}

%union {
    int num;
    char *str;
    astNode *node;
    std::vector<astNode*> *statement_list;
}

%token <num> NUM
%token <str> NAME
%token PRINT RETURN
%token EXTERN VOID INT IF ELSE WHILE READ
%token EQ NEQ LE GE
%type <node> expr
%type <node> statement
%type <statement_list> statement_list
%type <node> block

%left '+' '-'
%left '*' '/'

%start block
%%

block: 
    statement_list
        { $$ = createBlock($1); }
    | '{' statement_list '}' 
        { $$ = createBlock($2); }
    ;

statement_list:
    statement_list statement
        { $1->push_back($2); }
    | statement
        { $$->push_back($1); }
    ;

statement: 
    NAME '=' expr ';'
        { $$ = createAsgn(createVar($1), $3); }
    | PRINT '(' expr ')' ';'
        { $$ = createCall("print", $3); }
    | RETURN expr ';'
        { $$ = createRet($2); }
    | RETURN ';'
        { $$ = createRet(NULL); }
    ;

expr: 
    expr '+' expr
        { $$ = createBExpr($1, $3, add); }
    | expr '-' expr
        { $$ = createBExpr($1, $3, sub); }
    | expr '*' expr
        { $$ = createBExpr($1, $3, mul); }
    | expr '/' expr
        { $$ = createBExpr($1, $3, divide); }
    | '-' expr
        { $$ = createUExpr($2, sub); }
    | '+' expr
        { $$ = createUExpr($2, add); }
    | '*' expr
        { $$ = createUExpr($2, mul); }
    | '/' expr
        { $$ = createUExpr($2, divide); }
    | '(' expr ')'
        { $$ = $2; }
    | NUM
        { $$ = createCnst($1); }
    | NAME
        { $$ = createVar($1); }
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
