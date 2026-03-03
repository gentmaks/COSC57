/*
 * MiniC LLVM IR builder implementation.
 * Phase 1: preprocess AST and rename variables uniquely across scopes.
 * Phase 2: traverse AST and emit LLVM IR with the LLVM C API.
 */
#include "ir_builder.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct BuilderContext {
  LLVMModuleRef module = nullptr;
  LLVMBuilderRef builder = nullptr;
  LLVMValueRef currentFunction = nullptr;
  LLVMValueRef printFn = nullptr;
  LLVMValueRef readFn = nullptr;
  LLVMTypeRef printTy = nullptr;
  LLVMTypeRef readTy = nullptr;
  LLVMValueRef retAlloca = nullptr;
  LLVMBasicBlockRef retBB = nullptr;
  std::unordered_map<std::string, LLVMValueRef> varMap;
};

using RenameScope = std::unordered_map<std::string, std::string>;
using RenameStack = std::vector<RenameScope>;

// Allocate a writable C-string copy from a C++ string.
static char *dupCString(const std::string &s) {
  char *out = static_cast<char *>(std::malloc(s.size() + 1));
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}

// Replace an AST name field with a newly allocated renamed string.
static void overwriteName(char *&slot, const std::string &newName) {
  std::free(slot);
  slot = dupCString(newName);
}

// Return true if the basic block already has a terminator instruction.
static bool hasTerminator(LLVMBasicBlockRef bb) {
  return LLVMGetBasicBlockTerminator(bb) != nullptr;
}

// Look up the most recent renamed symbol for a variable use.
static std::string lookupRenamed(const char *name, const RenameStack &stack) {
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return found->second;
    }
  }
  return std::string(name);
}

// Create and record a unique name for a declaration in the current scope.
static std::string declareRenamed(char *&nameSlot, RenameStack &stack,
                                  int &counter) {
  std::string original(nameSlot);
  std::string renamed = original + "_" + std::to_string(counter++);
  stack.back()[original] = renamed;
  overwriteName(nameSlot, renamed);
  return renamed;
}

static void preprocessExpr(astNode *expr, RenameStack &stack, int &counter);
static void preprocessStmt(astNode *stmt, RenameStack &stack, int &counter);

// Preprocess a block by renaming declarations, then rewriting all uses.
static void preprocessBlock(astNode *blockNode, RenameStack &stack,
                            int &counter, bool createScope) {
  if (!blockNode || blockNode->type != ast_stmt ||
      blockNode->stmt.type != ast_block) {
    preprocessStmt(blockNode, stack, counter);
    return;
  }

  if (createScope) {
    stack.emplace_back();
  }

  std::vector<astNode *> &stmts = *blockNode->stmt.block.stmt_list;

  for (astNode *stmt : stmts) {
    if (stmt && stmt->type == ast_stmt && stmt->stmt.type == ast_decl) {
      declareRenamed(stmt->stmt.decl.name, stack, counter);
    }
  }

  for (astNode *stmt : stmts) {
    if (stmt && stmt->type == ast_stmt && stmt->stmt.type == ast_decl) {
      continue;
    }
    preprocessStmt(stmt, stack, counter);
  }

  if (createScope) {
    stack.pop_back();
  }
}

// Preprocess an expression subtree and rewrite variable references.
static void preprocessExpr(astNode *expr, RenameStack &stack, int &counter) {
  (void)counter;
  if (!expr) {
    return;
  }

  switch (expr->type) {
  case ast_var: {
    std::string renamed = lookupRenamed(expr->var.name, stack);
    overwriteName(expr->var.name, renamed);
    break;
  }
  case ast_bexpr:
    preprocessExpr(expr->bexpr.lhs, stack, counter);
    preprocessExpr(expr->bexpr.rhs, stack, counter);
    break;
  case ast_rexpr:
    preprocessExpr(expr->rexpr.lhs, stack, counter);
    preprocessExpr(expr->rexpr.rhs, stack, counter);
    break;
  case ast_uexpr:
    preprocessExpr(expr->uexpr.expr, stack, counter);
    break;
  case ast_stmt:
    if (expr->stmt.type == ast_call && expr->stmt.call.param) {
      preprocessExpr(expr->stmt.call.param, stack, counter);
    }
    break;
  default:
    break;
  }
}

// Preprocess a statement subtree and apply recursive renaming rules.
static void preprocessStmt(astNode *stmt, RenameStack &stack, int &counter) {
  if (!stmt) {
    return;
  }

  if (stmt->type != ast_stmt) {
    preprocessExpr(stmt, stack, counter);
    return;
  }

  switch (stmt->stmt.type) {
  case ast_block:
    preprocessBlock(stmt, stack, counter, true);
    break;
  case ast_asgn:
    if (stmt->stmt.asgn.lhs && stmt->stmt.asgn.lhs->type == ast_var) {
      std::string renamed = lookupRenamed(stmt->stmt.asgn.lhs->var.name, stack);
      overwriteName(stmt->stmt.asgn.lhs->var.name, renamed);
    }
    preprocessExpr(stmt->stmt.asgn.rhs, stack, counter);
    break;
  case ast_if:
    preprocessExpr(stmt->stmt.ifn.cond, stack, counter);
    preprocessStmt(stmt->stmt.ifn.if_body, stack, counter);
    preprocessStmt(stmt->stmt.ifn.else_body, stack, counter);
    break;
  case ast_while:
    preprocessExpr(stmt->stmt.whilen.cond, stack, counter);
    preprocessStmt(stmt->stmt.whilen.body, stack, counter);
    break;
  case ast_ret:
    preprocessExpr(stmt->stmt.ret.expr, stack, counter);
    break;
  case ast_call:
    preprocessExpr(stmt->stmt.call.param, stack, counter);
    break;
  case ast_decl:
  default:
    break;
  }
}

// Entry for preprocessing: start renaming from program root/function.
static void preprocessRoot(astNode *root) {
  if (!root || root->type != ast_prog || !root->prog.func) {
    return;
  }

  astNode *func = root->prog.func;
  RenameStack stack;
  stack.emplace_back();
  int counter = 0;

  if (func->func.param && func->func.param->type == ast_var) {
    declareRenamed(func->func.param->var.name, stack, counter);
  }

  preprocessBlock(func->func.body, stack, counter, false);
}

// Collect all declaration names to pre-create allocas in the entry block.
static void collectDeclNames(astNode *node, std::vector<std::string> &out) {
  if (!node) {
    return;
  }
  if (node->type != ast_stmt) {
    return;
  }

  switch (node->stmt.type) {
  case ast_decl:
    out.emplace_back(node->stmt.decl.name);
    break;
  case ast_block: {
    for (astNode *child : *node->stmt.block.stmt_list) {
      collectDeclNames(child, out);
    }
    break;
  }
  case ast_if:
    collectDeclNames(node->stmt.ifn.if_body, out);
    collectDeclNames(node->stmt.ifn.else_body, out);
    break;
  case ast_while:
    collectDeclNames(node->stmt.whilen.body, out);
    break;
  default:
    break;
  }
}

// Convert AST relational operator enum into the LLVM comparison predicate.
static LLVMIntPredicate mapRelop(rop_type op) {
  switch (op) {
  case lt:
    return LLVMIntSLT;
  case gt:
    return LLVMIntSGT;
  case le:
    return LLVMIntSLE;
  case ge:
    return LLVMIntSGE;
  case eq:
    return LLVMIntEQ;
  case neq:
    return LLVMIntNE;
  default:
    return LLVMIntEQ;
  }
}

static LLVMValueRef genIRExpr(astNode *expr, BuilderContext &ctx);

// Generate IR for one statement subtree and return the trailing basic block.
static LLVMBasicBlockRef genIRStmt(astNode *stmt, LLVMBasicBlockRef startBB,
                                   BuilderContext &ctx) {
  if (!stmt) {
    return startBB;
  }

  if (stmt->type != ast_stmt) {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    (void)genIRExpr(stmt, ctx);
    return startBB;
  }

  switch (stmt->stmt.type) {
  case ast_asgn: {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    LLVMValueRef rhs = genIRExpr(stmt->stmt.asgn.rhs, ctx);
    const std::string lhsName(stmt->stmt.asgn.lhs->var.name);
    LLVMBuildStore(ctx.builder, rhs, ctx.varMap[lhsName]);
    return startBB;
  }
  case ast_call: {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    if (std::strcmp(stmt->stmt.call.name, "print") == 0) {
      LLVMValueRef arg = genIRExpr(stmt->stmt.call.param, ctx);
      LLVMValueRef args[] = {arg};
      LLVMBuildCall2(ctx.builder, ctx.printTy, ctx.printFn, args, 1, "");
    }
    return startBB;
  }
  case ast_ret: {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    LLVMValueRef retVal = genIRExpr(stmt->stmt.ret.expr, ctx);
    LLVMBuildStore(ctx.builder, retVal, ctx.retAlloca);
    LLVMBuildBr(ctx.builder, ctx.retBB);
    LLVMBasicBlockRef afterRet =
        LLVMAppendBasicBlock(ctx.currentFunction, "after_ret");
    return afterRet;
  }
  case ast_if: {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    LLVMValueRef cond = genIRExpr(stmt->stmt.ifn.cond, ctx);
    LLVMBasicBlockRef trueBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "if_t");
    LLVMBasicBlockRef falseBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "if_f");
    LLVMBuildCondBr(ctx.builder, cond, trueBB, falseBB);

    LLVMBasicBlockRef ifExit = genIRStmt(stmt->stmt.ifn.if_body, trueBB, ctx);
    if (!hasTerminator(ifExit)) {
      LLVMPositionBuilderAtEnd(ctx.builder, ifExit);
    }

    if (!stmt->stmt.ifn.else_body) {
      if (!hasTerminator(ifExit)) {
        LLVMBuildBr(ctx.builder, falseBB);
      }
      return falseBB;
    }

    LLVMBasicBlockRef elseExit =
        genIRStmt(stmt->stmt.ifn.else_body, falseBB, ctx);
    LLVMBasicBlockRef endBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "if_end");

    if (!hasTerminator(ifExit)) {
      LLVMPositionBuilderAtEnd(ctx.builder, ifExit);
      LLVMBuildBr(ctx.builder, endBB);
    }
    if (!hasTerminator(elseExit)) {
      LLVMPositionBuilderAtEnd(ctx.builder, elseExit);
      LLVMBuildBr(ctx.builder, endBB);
    }
    return endBB;
  }
  case ast_while: {
    LLVMPositionBuilderAtEnd(ctx.builder, startBB);
    LLVMBasicBlockRef condBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "while_cond");
    LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, condBB);
    LLVMValueRef cond = genIRExpr(stmt->stmt.whilen.cond, ctx);
    LLVMBasicBlockRef trueBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "while_body");
    LLVMBasicBlockRef falseBB =
        LLVMAppendBasicBlock(ctx.currentFunction, "while_exit");
    LLVMBuildCondBr(ctx.builder, cond, trueBB, falseBB);

    LLVMBasicBlockRef bodyExit = genIRStmt(stmt->stmt.whilen.body, trueBB, ctx);
    if (!hasTerminator(bodyExit)) {
      LLVMPositionBuilderAtEnd(ctx.builder, bodyExit);
      LLVMBuildBr(ctx.builder, condBB);
    }
    return falseBB;
  }
  case ast_block: {
    LLVMBasicBlockRef prev = startBB;
    for (astNode *child : *stmt->stmt.block.stmt_list) {
      prev = genIRStmt(child, prev, ctx);
    }
    return prev;
  }
  case ast_decl:
  default:
    return startBB;
  }
}

// Generate IR value for an expression subtree.
static LLVMValueRef genIRExpr(astNode *expr, BuilderContext &ctx) {
  if (!expr) {
    return LLVMConstInt(LLVMInt32Type(), 0, 1);
  }

  switch (expr->type) {
  case ast_cnst:
    return LLVMConstInt(LLVMInt32Type(), expr->cnst.value, 1);
  case ast_var: {
    const std::string name(expr->var.name);
    return LLVMBuildLoad2(ctx.builder, LLVMInt32Type(), ctx.varMap[name], "");
  }
  case ast_uexpr: {
    LLVMValueRef inner = genIRExpr(expr->uexpr.expr, ctx);
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 1);
    return LLVMBuildSub(ctx.builder, zero, inner, "");
  }
  case ast_bexpr: {
    LLVMValueRef lhs = genIRExpr(expr->bexpr.lhs, ctx);
    LLVMValueRef rhs = genIRExpr(expr->bexpr.rhs, ctx);
    switch (expr->bexpr.op) {
    case add:
      return LLVMBuildAdd(ctx.builder, lhs, rhs, "");
    case sub:
      return LLVMBuildSub(ctx.builder, lhs, rhs, "");
    case mul:
      return LLVMBuildMul(ctx.builder, lhs, rhs, "");
    case divide:
      return LLVMBuildSDiv(ctx.builder, lhs, rhs, "");
    default:
      return lhs;
    }
  }
  case ast_rexpr: {
    LLVMValueRef lhs = genIRExpr(expr->rexpr.lhs, ctx);
    LLVMValueRef rhs = genIRExpr(expr->rexpr.rhs, ctx);
    return LLVMBuildICmp(ctx.builder, mapRelop(expr->rexpr.op), lhs, rhs, "");
  }
  case ast_stmt: {
    if (expr->stmt.type == ast_call &&
        std::strcmp(expr->stmt.call.name, "read") == 0) {
      return LLVMBuildCall2(ctx.builder, ctx.readTy, ctx.readFn, nullptr, 0,
                            "");
    }
    break;
  }
  default:
    break;
  }

  return LLVMConstInt(LLVMInt32Type(), 0, 1);
}

// Create miniC extern function declarations for print/read.
static void createExterns(astNode *progNode, BuilderContext &ctx) {
  (void)progNode;
  LLVMTypeRef i32 = LLVMInt32Type();
  LLVMTypeRef voidTy = LLVMVoidType();

  LLVMTypeRef printArgs[] = {i32};
  ctx.printTy = LLVMFunctionType(voidTy, printArgs, 1, 0);
  ctx.printFn = LLVMAddFunction(ctx.module, "print", ctx.printTy);

  ctx.readTy = LLVMFunctionType(i32, nullptr, 0, 0);
  ctx.readFn = LLVMAddFunction(ctx.module, "read", ctx.readTy);
}

// Build IR for the single miniC function using entry allocas and ret block.
static void buildFunctionIR(astNode *funcNode, BuilderContext &ctx) {
  LLVMTypeRef i32 = LLVMInt32Type();
  LLVMTypeRef paramTypes[1];
  unsigned paramCount = 0;
  if (funcNode->func.param) {
    paramTypes[0] = i32;
    paramCount = 1;
  }

  LLVMTypeRef funcTy = LLVMFunctionType(i32, paramTypes, paramCount, 0);
  ctx.currentFunction =
      LLVMAddFunction(ctx.module, funcNode->func.name, funcTy);

  LLVMBasicBlockRef entryBB =
      LLVMAppendBasicBlock(ctx.currentFunction, "entry");
  ctx.retBB = LLVMAppendBasicBlock(ctx.currentFunction, "ret");
  LLVMPositionBuilderAtEnd(ctx.builder, entryBB);

  ctx.varMap.clear();
  std::vector<std::string> locals;
  collectDeclNames(funcNode->func.body, locals);
  for (const std::string &name : locals) {
    LLVMValueRef alloc = LLVMBuildAlloca(ctx.builder, i32, name.c_str());
    ctx.varMap[name] = alloc;
  }

  if (funcNode->func.param && funcNode->func.param->type == ast_var) {
    const std::string paramName(funcNode->func.param->var.name);
    LLVMValueRef paramAlloc =
        LLVMBuildAlloca(ctx.builder, i32, paramName.c_str());
    ctx.varMap[paramName] = paramAlloc;
    LLVMBuildStore(ctx.builder, LLVMGetParam(ctx.currentFunction, 0),
                   paramAlloc);
  }

  ctx.retAlloca = LLVMBuildAlloca(ctx.builder, i32, "retval.addr");

  LLVMBasicBlockRef exitBB = genIRStmt(funcNode->func.body, entryBB, ctx);
  if (!hasTerminator(exitBB)) {
    LLVMPositionBuilderAtEnd(ctx.builder, exitBB);
    LLVMBuildBr(ctx.builder, ctx.retBB);
  }

  LLVMPositionBuilderAtEnd(ctx.builder, ctx.retBB);
  LLVMValueRef retVal =
      LLVMBuildLoad2(ctx.builder, i32, ctx.retAlloca, "retval");
  LLVMBuildRet(ctx.builder, retVal);
}

} // namespace

// Public API: preprocess AST and generate an LLVM module for the program.
LLVMModuleRef buildIRModule(astNode *root) {
  if (!root || root->type != ast_prog || !root->prog.func) {
    return nullptr;
  }

  preprocessRoot(root);

  BuilderContext ctx;
  ctx.module = LLVMModuleCreateWithName("minic_module");
  LLVMSetTarget(ctx.module, "x86_64-pc-linux-gnu");
  ctx.builder = LLVMCreateBuilder();

  createExterns(root, ctx);
  buildFunctionIR(root->prog.func, ctx);

  LLVMDisposeBuilder(ctx.builder);
  return ctx.module;
}
