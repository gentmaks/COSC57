---
name: miniC lex+yacc AST
overview: Implement a C-like miniC lexer+parser using flex+bison, building your existing AST nodes (`frontend/ast/ast.h`) directly in bison semantic actions with correct operator precedence, statement/block structure, and the fixed `extern print/read` prologue.
todos:
  - id: define-tokens-lexer
    content: Update `frontend/minic.l` to recognize miniC keywords, multi-char operators, NUM/NAME, and return correct tokens/characters.
    status: pending
  - id: bison-union-types
    content: Expand `frontend/minic.y` `%union` and `%type` declarations to carry `astNode*`, vectors of statements, and operator enums.
    status: pending
    dependencies:
      - define-tokens-lexer
  - id: grammar-ast-actions
    content: Implement program/extern/function/block/stmt/expr grammar with precedence + dangling-else handling, building AST via `create*()` in semantic actions.
    status: pending
    dependencies:
      - bison-union-types
  - id: build-integration
    content: Adjust `frontend/Makefile` to compile as C++ and link `frontend/ast/ast.c` (or rename to .cpp / use `-x c++`).
    status: pending
    dependencies:
      - grammar-ast-actions
  - id: smoke-tests
    content: Add/collect a small set of miniC input files to validate parsing + AST printing for each feature (decl ordering, precedence, if-else, while, read/print, return).
    status: pending
    dependencies:
      - build-integration
---

# miniC parser+AST plan (flex + bison)

## Goal

Build a **C-like** miniC parser that:

- Recognizes the language you described (no comments, `int` only, one function, `if/if-else/while/block/decl/asgn/print/read/return`).
- Builds an **AST** using your existing constructors in [`/Users/gent/Desktop/compilers/frontend/ast/ast.h`](/Users/gent/Desktop/compilers/frontend/ast/ast.h) and [`/Users/gent/Desktop/compilers/frontend/ast/ast.c`](/Users/gent/Desktop/compilers/frontend/ast/ast.c).

## High-level structure (matches your “Program → Block → …” intent)

Use a layered grammar (this is the standard, conflict-free way to achieve precedence):

```text
program
  -> extern_print extern_read func

func
  -> "int" NAME '(' opt_param ')' block

block
  -> '{' decls stmts '}'

stmt
  -> if_stmt | while_stmt | return_stmt | assign_stmt | print_stmt | block

expr (lowest)
  -> rel_expr | add_expr

add_expr
  -> add_expr ('+'|'-') mul_expr | mul_expr

mul_expr
  -> mul_expr ('*'|'/') unary_expr | unary_expr

unary_expr
  -> '-' unary_expr | primary

primary (your “term”)
  -> NUM | NAME | '(' expr ')'
```

This gives you the “Program → Block → statement → expression → term” shape while still handling precedence correctly.

## 1) Lexer plan: [`/Users/gent/Desktop/compilers/frontend/minic.l`](/Users/gent/Desktop/compilers/frontend/minic.l)

### Tokens to produce

- **Keywords**: `extern`, `void`, `int`, `if`, `else`, `while`, `return`, `print`, `read`
- **Multi-char operators**: `== != <= >=`
- **Single-char punctuation/operators** (can be returned as their character): `{ } ( ) ; , = + - * / < >`
- **Literals**: integer constants (`NUM`)
- **Identifiers**: variable/function names (`NAME`)

### Lexer structure

- Put **multi-char operators first** (`==`, `!=`, `<=`, `>=`) so they don’t get split.
- Put **keywords before** the identifier rule, so `if` doesn’t become `NAME`.
- Use a C-like identifier regex: `[a-zA-Z_][a-zA-Z0-9_]*`.
- Keep your current approach for values:
  - `NUM`: `yylval.num = atoi(yytext)`
  - `NAME`: `yylval.str = strdup(yytext)`

### Memory rule (important)

Because you use `strdup` in the lexer and your AST `create*` functions **copy** strings internally (`createVar`, `createDecl`, `createCall`, `createFunc`), you should **free** the token string in bison actions (or use a `%destructor` for `<str>`).

### Error handling

Any unexpected character should return itself (or call `yyerror`). Since miniC has “no comments”, you don’t need comment-skipping rules.

## 2) Parser plan: [`/Users/gent/Desktop/compilers/frontend/minic.y`](/Users/gent/Desktop/compilers/frontend/minic.y)

### 2.1 Bison C/C++ integration (because your AST uses `vector`)

Your AST header uses C++ (`<vector>`, `new`, default args). So the parser must be compiled as **C++**.

- Easiest path: compile/link everything with `g++` (or make `ast.c` compile as C++ via `-x c++`, or rename it to `ast.cpp`).

### 2.2 `%union` and `%type`

Expand `%union` so nonterminals can carry AST pointers and statement lists.

- Add:
  - `astNode* node;`
  - `vector<astNode*>* vec;`
  - `rop_type rop;` (for relational operator kind)
  - `op_type op;` (for arithmetic operator kind)

Then declare:

- `%type <node> program func block stmt expr rel_expr add_expr mul_expr unary_expr primary rhs` etc.
- `%type <vec> decls stmts`.
- `%type <rop> relop`.

### 2.3 Operator precedence

Declare precedence in bison (so expression parsing is clean):

- Lowest among these: relational
- Then `+ -`
- Then `* /`
- Unary minus highest

Also handle the classic **dangling else**:

- Use `%nonassoc LOWER_THAN_ELSE` and `%nonassoc ELSE`
- Make the `if (..) stmt` rule use `%prec LOWER_THAN_ELSE`

### 2.4 Program + extern prologue

Parse exactly the required prologue:

- `extern void print(int [optionalName]) ;`
- `extern int read() ;`

AST creation:

- `extern_print` builds `createExtern("print")`
- `extern_read` builds `createExtern("read")`
- `program` builds `createProg(ext_print, ext_read, func)`

### 2.5 Function (exactly one)

Rules:

- `int NAME ( ) block`
- `int NAME ( int NAME ) block`

AST:

- No-param: `createFunc($2, NULL, $block)`
- One-param: `createFunc($2, createVar($paramName), $block)`
- Free the lexer strings after building nodes.

### 2.6 Block with “decls first” rule

Enforce: “All declaration statements in a block are at the start”.

Grammar:

- `block: '{' decls stmts '}'`
- `decls: decls decl | /* empty */`
- `decl: INT NAME ';'`
- `stmts: stmts stmt | /* empty */`

AST strategy:

- Create a single `vector<astNode*>*` for the block.
- Push decl nodes first (as `createDecl(name)`), then push statement nodes.
- Finally: `createBlock(stmt_list_vec)`

### 2.7 Statements (build AST in actions)

Implement these as `astNode*` (node type `ast_stmt`):

- **If**:
  - `IF '(' expr ')' stmt`
  - `IF '(' expr ')' stmt ELSE stmt`
  - Build: `createIf(cond, ifBody, elseBodyOrNULL)`
- **While**:
  - `WHILE '(' expr ')' stmt`
  - Build: `createWhile(cond, body)`
- **Assignment**:
  - `NAME '=' rhs ';'`
  - LHS node: `createVar(name)`
  - Build: `createAsgn(lhsVarNode, rhsNode)`
- **RHS**:
  - `rhs: expr | READ '(' ')'`
  - For `read()`, build `createCall("read", NULL)`
    - Note: your AST represents calls as `ast_stmt` nodes; this still works as assignment RHS because `createAsgn` stores an `astNode*` and `freeNode` handles `ast_stmt`.
- **Print call statement**:
  - `PRINT '(' expr ')' ';'`
  - Build: `createCall("print", expr)`
- **Return**:
  - `RETURN expr ';'`
  - Build: `createRet(expr)`
- **Block** statement:
  - `stmt: block` (nested blocks automatically work)

### 2.8 Expressions (2-operand tree, unary minus)

Even though the spec says “all expressions only have 2 operands”, that’s naturally satisfied because each operator node (`createBExpr`/`createRExpr`) has exactly 2 children.

- **Relational**:
  - `rel_expr: add_expr relop add_expr | add_expr`
  - `relop` maps tokens to `rop_type` (`lt/gt/le/ge/eq/neq`)
  - If relational used: `createRExpr(lhs, rhs, rop)`
- **Arithmetic**:
  - `add_expr: add_expr '+' mul_expr | add_expr '-' mul_expr | mul_expr`
  - `mul_expr: mul_expr '*' unary_expr | mul_expr '/' unary_expr | unary_expr`
  - Build `createBExpr(lhs, rhs, add/sub/mul/divide)`
- **Unary**:
  - `unary_expr: '-' unary_expr %prec UMINUS | primary`
  - Build `createUExpr(expr, uminus)`
- **Primary (term)**:
  - `NUM` → `createCnst(value)`
  - `NAME` → `createVar(name)`
  - `'(' expr ')'` → pass through

## 3) Main driver behavior

In `minic.y`:

- Have a global `astNode* root` set in the `program` rule.
- In `main()`: call `yyparse()`, then `printNode(root)` (you already have `printNode`), then `freeNode(root)`.

## 4) Build system adjustments (so this actually links)

Your current [`/Users/gent/Desktop/compilers/frontend/Makefile`](/Users/gent/Desktop/compilers/frontend/Makefile) uses `gcc` and does not link `ast/ast.c`.

Plan to update it so:

- It compiles with **C++** (because `frontend/ast/ast.h` uses `vector` and `new`).
- It links in `frontend/ast/ast.c`.

Two acceptable approaches:

- **Approach A (minimal file rename)**: keep file names, but compile AST as C++: add `-x c++ frontend/ast/ast.c` when compiling/linking.
- **Approach B (clean)**: rename `frontend/ast/ast.c` → `frontend/ast/ast.cpp` and compile/link with `g++`.

Also, if you want bison explicitly but keep `y.tab.c/h` naming:

- Use `bison -y -d minic.y` (the `-y` makes it behave like `yacc` output names).

## 5) Test plan (small inputs to validate each rule)

Create tiny programs that isolate features:

- **Extern + empty function with return**
- **Local decls then stmts in block** (and reject decl after stmt)
- **Assignment from `read()`**
- **`print(expr)`**
- **Arithmetic precedence**: `return 1+2*3;` vs `(1+2)*3`
- **Unary minus**: `return -x; return -(1+2);`
- **Relational** in `if`/`while`: `if (x<=3) return x; else return 0;`
- **Dangling else** nesting: `if(a) if(b) print(1); else print(2);` should bind `else` to inner `if`.

## 6) Deliverables

- A revised [`/Users/gent/Desktop/compilers/frontend/minic.l`](/Users/gent/Desktop/compilers/frontend/minic.l) that returns proper tokens for keywords and multi-char operators.
- A revised [`/Users/gent/Desktop/compilers/frontend/minic.y`](/Users/gent/Desktop/compilers/frontend/minic.y) with:
  - `%union` carrying AST pointers and vectors
  - precedence declarations
  - grammar rules for externs, function, blocks/decls/stmts, expressions
  - semantic actions calling `create*` functions
- A build update so `minic` links with `frontend/ast/ast.c` and compiles as C++.