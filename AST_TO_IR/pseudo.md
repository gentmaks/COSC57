# AST to IR for minic programs

Input: AST root (`ast_prog`)  
Output: LLVM module (`LLVMModuleRef`)

## Shared state

- `module`, `builder`, `currentFunction`
- `printFn`, `readFn` (the only extern calls in miniC)
- `varAllocaMap` : variable name -> alloca pointer
- `retAlloca`, `retBB` (single return strategy)

## Top-level flow

- Create module and builder.
- Call `genIRNode(root)`.
- Print module to file and dispose LLVM objects.

## `genIRNode(node)`

- If `ast_prog`:
  - Generate `ext1` (print), `ext2` (read), then `func`.

- If `ast_extern`:
  - If name is `print`: create `void(i32)` via `LLVMFunctionType` + `LLVMAddFunction`, store in `printFn`.
  - If name is `read`: create `i32()` similarly, store in `readFn`.

- If `ast_func`:
  - Create function (one user function since miniC with 0/1 args passed in).
  - Create entry block and position builder there.
  - Clear `varAllocaMap`.
  - Create `retAlloca` and `retBB`.
  - For each parameter:
    - alloca + store incoming parameter.
    - save alloca in `varAllocaMap`.
  - Generate body using `genIRStmt`.
  - If current block has no terminator, branch to `retBB`.
  - In `retBB`: load `retAlloca` and emit `ret`.

## `genIRStmt(stmt)`

- If `ast_decl`:
  - Create alloca and record in `varAllocaMap`.

- If `ast_asgn`:
  - Evaluate RHS with `genIRExpr`.
  - Lookup LHS pointer from `varAllocaMap`.
  - Emit store.

- If `ast_call`:
  - If name is `print`:
    - Evaluate one argument.
    - Emit `LLVMBuildCall2` with `printFn`.
  - If name is `read`:
    - Emit `LLVMBuildCall2` with `readFn` and no args.
    - Use returned value only when this call appears as expression (e.g., RHS of assignment).

- If `ast_ret`:
  - Evaluate return expression.
  - Store into `retAlloca`.
  - Branch to `retBB`.

- If `ast_block`:
  - Generate child statements in order.
  - Stop early if current block already has terminator.

- If `ast_if`:
  - Evaluate condition (must become i1).
  - Create `ifBB`, optional `elseBB`, and `mergeBB`.
  - Emit conditional branch.
  - Generate `if` body, branch to merge if needed.
  - Generate `else` body (if present), branch to merge if needed.
  - Move builder to `mergeBB`.

- If `ast_while`:
  - Create `condBB`, `bodyBB`, `exitBB`.
  - Branch to `condBB`.
  - In `condBB`: evaluate condition and emit conditional branch.
  - In `bodyBB`: generate loop body, emit back-edge to `condBB` if needed.
  - Move builder to `exitBB`.

## `genIRExpr(expr)`

- If `ast_cnst`: return `LLVMConstInt(i32, value, 1)`.
- If `ast_var`: load from `varAllocaMap[name]` using `LLVMBuildLoad2`.
- If `ast_bexpr`: recursively evaluate lhs/rhs, emit `Add/Sub/Mul/SDiv`.
- If `ast_uexpr` (`uminus`): recursively evaluate operand, emit `LLVMBuildNeg`.
- If `ast_rexpr`: recursively evaluate lhs/rhs, emit `LLVMBuildICmp` with mapped predicate.
- If expression is `read()` call: emit `LLVMBuildCall2(readFn, 0 args)` and return result.