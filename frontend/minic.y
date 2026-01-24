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
    '{' statement_list '}' 
        { 
        $$ = createBlock($2); 
        printf("Statement List with brackets\n");
        }
    | statement_list
        { 
        $$ = createBlock($1); 
        printf("Statement List\n");
        }
    ;

statement_list:
    statement_list statement
        { 
        $1->push_back($2); $$ = $1; 
        printf("Statement appended to the statement list\n");
        }
    | statement
        { 
        $$ = new std::vector<astNode*>; $$->push_back($1); 
        printf("Statement created initial\n");
        }
    ;

statement: 
    INT NAME ';'
        { 
        $$ = createDecl($2); 
        printf("Declaration matched with type\n");
        }
    | INT NAME '=' expr ';'
        { 
        $$ = createAsgn(createDecl($2), $4); 
        printf("Declaration with initialization\n");
        }
    | NAME '=' expr ';'
        { 
        $$ = createAsgn(createVar($1), $3); 
        printf("Assignment created\n");
        }
    | PRINT '(' expr ')' ';'
        { 
        $$ = createCall("print", $3); 
        printf("Print function called\n");
        }
    | RETURN expr ';'
        { 
        $$ = createRet($2); 
        printf("Return statement\n");
        }
    | RETURN ';'
        { 
        $$ = createRet(NULL); 
        printf("Empty return statement\n");
        }
    | '{' statement_list '}'
        { 
        $$ = createBlock($2); 
        printf("Nested block\n");
        }
    ;

expr:
    expr '+' expr
        { 
        $$ = createBExpr($1, $3, add);
        printf("add expression created\n");
        }
    | expr '-' expr
        { 
        $$ = createBExpr($1, $3, sub); 
        printf("sub expression created\n");
        }
    | expr '*' expr
        { 
        $$ = createBExpr($1, $3, mul); 
        printf("mul expression created\n");
        }
    | expr '/' expr
        { 
        $$ = createBExpr($1, $3, divide); 
        printf("divide expression created\n");
        }
    | '-' expr
        { 
        $$ = createUExpr($2, sub); 
        printf("unary - expression created\n");
        }
    | '+' expr
        { 
        $$ = createUExpr($2, add); 
        printf("unary + expression created\n");
        }
    | '*' expr
        { 
        $$ = createUExpr($2, mul); 
        printf("unary * expression created\n");
        }
    | '/' expr
        { 
        $$ = createUExpr($2, divide); 
        printf("unary / expression created\n");
        }
    | '(' expr ')'
        { 
        $$ = $2; 
        printf("expression inside parens\n");
        }
    | NUM
        { 
        $$ = createCnst($1); 
        printf("simple num matched\n");
        }
    | NAME
        { 
        $$ = createVar($1); 
        printf("simple name matched\n");
        }
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
