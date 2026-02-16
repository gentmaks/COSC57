#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <llvm-c/Core.h>
#include <stdbool.h>

LLVMModuleRef createLLVMModuleFromFile(const char *filename);
bool optimizeFunction(LLVMValueRef function);

bool runCommonSubexpressionElimination(LLVMValueRef function);
bool runConstantFolding(LLVMValueRef function);
bool runDeadCodeElimination(LLVMValueRef function);
bool runStoreLoadConstantPropagation(LLVMValueRef function);

#endif
