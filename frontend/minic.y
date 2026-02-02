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
%type <node> program
%type <node> extern_print extern_read
%type <node> function param_opt
%type <node> block
%type <node> statement
%type <node> assignment_rhs
%type <node> expr
%type <node> rel_expr
%type <statement_list> decl_list
%type <statement_list> stmt_list

%left '+' '-'
%left '*' '/'
%right UMINUS
%nonassoc IFX
%nonassoc ELSE

%start program
%%

program:
    extern_print extern_read function
        {
        $$ = createProg($1, $2, $3);
        rootNode = $$;
        }
    ;

extern_print:
    EXTERN VOID PRINT '(' INT ')' ';'
        {
        $$ = createExtern("print");
        }
    ;

extern_read:
    EXTERN INT READ '(' ')' ';'
        {
        $$ = createExtern("read");
        }
    ;

function:
    INT NAME '(' param_opt ')' block
        {
        $$ = createFunc($2, $4, $6);
        }
    ;

param_opt:
    INT NAME
        {
        $$ = createVar($2);
        }
    | /* empty */
        {
        $$ = NULL;
        }
    ;

block:
    '{' decl_list stmt_list '}'
        {
        if ($2->empty()) {
            delete $2;
            $$ = createBlock($3);
        } else {
            $2->insert($2->end(), $3->begin(), $3->end());
            delete $3;
            $$ = createBlock($2);
        }
        }
    ;

decl_list:
    decl_list INT NAME ';'
        {
        $1->push_back(createDecl($3));
        $$ = $1;
        }
    | /* empty */
        {
        $$ = new std::vector<astNode*>;
        }
    ;

stmt_list:
    stmt_list statement
        {
        $1->push_back($2);
        $$ = $1;
        }
    | /* empty */
        {
        $$ = new std::vector<astNode*>;
        }
    ;

statement:
    NAME '=' assignment_rhs ';'
        {
        $$ = createAsgn(createVar($1), $3);
        }
    | PRINT '(' expr ')' ';'
        {
        $$ = createCall("print", $3);
        }
    | RETURN expr ';'
        {
        $$ = createRet($2);
        }
    | IF '(' rel_expr ')' statement %prec IFX
        {
        $$ = createIf($3, $5);
        }
    | IF '(' rel_expr ')' statement ELSE statement
        {
        $$ = createIf($3, $5, $7);
        }
    | WHILE '(' rel_expr ')' statement
        {
        $$ = createWhile($3, $5);
        }
    | block
        {
        $$ = $1;
        }
    ;

assignment_rhs:
    expr
        {
        $$ = $1;
        }
    | READ '(' ')'
        {
        $$ = createCall("read", NULL);
        }
    ;

rel_expr:
    expr '<' expr
        {
        $$ = createRExpr($1, $3, lt);
        }
    | expr '>' expr
        {
        $$ = createRExpr($1, $3, gt);
        }
    | expr LE expr
        {
        $$ = createRExpr($1, $3, le);
        }
    | expr GE expr
        {
        $$ = createRExpr($1, $3, ge);
        }
    | expr EQ expr
        {
        $$ = createRExpr($1, $3, eq);
        }
    | expr NEQ expr
        {
        $$ = createRExpr($1, $3, neq);
        }
    ;

expr:
    expr '+' expr
        {
        $$ = createBExpr($1, $3, add);
        }
    | expr '-' expr
        {
        $$ = createBExpr($1, $3, sub);
        }
    | expr '*' expr
        {
        $$ = createBExpr($1, $3, mul);
        }
    | expr '/' expr
        {
        $$ = createBExpr($1, $3, divide);
        }
    | '-' expr %prec UMINUS
        {
        $$ = createUExpr($2, uminus);
        }
    | '(' expr ')'
        {
        $$ = $2;
        }
    | NUM
        {
        $$ = createCnst($1);
        }
    | NAME
        {
        $$ = createVar($1);
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
