/*
 * MiniC LLVM IR builder interface.
 * Exposes a single entry point that takes an AST root and returns an LLVM
 * module produced by the IR builder pipeline (preprocess + IR generation).
 */
#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "../frontend/ast.h"
#include <llvm-c/Core.h>

// Build LLVM IR module from the parsed miniC AST root.
LLVMModuleRef buildIRModule(astNode *root);

#endif
