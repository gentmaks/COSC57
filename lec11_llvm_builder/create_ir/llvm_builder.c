#include <llvm-c/Core.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
	//Creating a module 
  LLVMModuleRef mod = LLVMModuleCreateWithName("");
	//LLVMSetTarget(mod, "x86_64-pc-linux-gnu");
	
	//Creating a function with a module
  LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
  LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
  LLVMValueRef func = LLVMAddFunction(mod, "test", ret_type);

	//Creating a basic block
  LLVMBasicBlockRef first = LLVMAppendBasicBlock(func, "");

	//All instructions need to be created using a builder. The builder specifies
	//where the instructions are added.
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, first);

	//Creating an alloc instruction with alignment
	LLVMValueRef m = LLVMBuildAlloca(builder, LLVMInt32Type(), "m"); 
	LLVMSetAlignment(m, 4);

	//Creating an alloc without alignment. 
	//Not specifying an alignment doesn't make alloc incorrect. 
	//But in some cases it might be inefficient.
	LLVMValueRef n = LLVMBuildAlloca(builder, LLVMInt32Type(), "n"); 
	LLVMSetAlignment(n, 4);
	LLVMValueRef a = LLVMBuildAlloca(builder, LLVMInt32Type(), "a"); 
	LLVMSetAlignment(a, 4);

	//Creating store instructions to make local copy of parameters
	LLVMBuildStore(builder, LLVMGetParam(func, 0), m);
	LLVMBuildStore(builder, LLVMGetParam(func, 1), n);

	//Creating store instructions for local variables
	LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1), a);
	

	//Creating load instructions. Do not use LLVMBuildLoad instead use LLVMBuildLoad2
	LLVMValueRef l1 = LLVMBuildLoad2(builder, LLVMInt32Type(), m, "");	
	LLVMValueRef l2 = LLVMBuildLoad2(builder, LLVMInt32Type(), n, "");	

	//Creating an arithmetic instruction. In this case it is add instruction
	LLVMValueRef t3 = LLVMBuildICmp(builder, LLVMIntSLT, l1, l2, "");

	//Creating basic blocks for if and then
	
  LLVMBasicBlockRef if_BB = LLVMAppendBasicBlock(func, "");
  LLVMBasicBlockRef else_BB = LLVMAppendBasicBlock(func, "");
  LLVMBasicBlockRef final = LLVMAppendBasicBlock(func, "");
	
	
	LLVMBuildCondBr(builder, t3, if_BB, else_BB);

	//Creating instuctions in if_BB
  LLVMPositionBuilderAtEnd(builder, if_BB);
	
	LLVMValueRef t4 = LLVMBuildLoad2(builder, LLVMInt32Type(), m, "");
	LLVMBuildStore(builder, t4, a);
	
	LLVMBuildBr(builder, final);

	//Creating instructions in else_BB	
  LLVMPositionBuilderAtEnd(builder, else_BB);

	LLVMValueRef t5 = LLVMBuildLoad2(builder, LLVMInt32Type(), n, "");
	LLVMBuildStore(builder, t5, a);
	
	LLVMBuildBr(builder, final);


	//Creating instructions in final basic block
	
  LLVMPositionBuilderAtEnd(builder, final);

	LLVMValueRef t6 = LLVMBuildLoad2(builder, LLVMInt32Type(), a, "");
  LLVMBuildRet(builder, t6);

	//Writing out the LLVM IR
	LLVMDumpModule(mod);
	LLVMPrintModuleToFile (mod, "test.ll", NULL);

	//Cleanup
  LLVMDisposeBuilder(builder);
	LLVMDisposeModule(mod);
}
