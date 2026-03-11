/*
Author: Gent Maksutaj
Description: Interface for the assembly generation backend.
It exposes helpers to parse LLVM IR and emit x86 32-bit assembly.
*/
#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H

#include <llvm-c/Core.h>

// Parse an LLVM IR module from a .ll file.
LLVMModuleRef createLLVMModuleFromFile(const char *filename);
// Emit x86 assembly for all defined functions in the module.
void generateAssemblyForModule(LLVMModuleRef module, const char *outputPath);

#endif
