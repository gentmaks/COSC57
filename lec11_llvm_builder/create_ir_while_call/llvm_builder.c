#include <llvm-c/Core.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
	//Creating a module 
  LLVMModuleRef mod = LLVMModuleCreateWithName("");
	LLVMSetTarget(mod, "x86_64-pc-linux-gnu");
	
	//Creating extern function declaration. A function without a body is treated as declaration.
  LLVMTypeRef param_read[] = {};
  LLVMTypeRef ret_read = LLVMFunctionType(LLVMInt32Type(), param_read, 0, 0);
  LLVMValueRef func_read = LLVMAddFunction(mod, "read", ret_read);

  //Creating a function in a module
  LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
  LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
  LLVMValueRef func = LLVMAddFunction(mod, "test", func_type);

	//Creating a basic block
  LLVMBasicBlockRef first = LLVMAppendBasicBlock(func, "");

	//All instructions need to be created using a builder. The builder specifies
	//where the instructions are added.
  LLVMBuilderRef builder = LLVMCreateBuilder();
  // This positions the builder in the end of the first basicblock. There are other position functions in Core.h
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

  //Creating a call to read
	LLVMValueRef *read_args = NULL;
	LLVMValueRef read_call = LLVMBuildCall2(builder, ret_read, func_read, read_args, 0, ""); 

	//Creating store instructions for local variables
	LLVMBuildStore(builder, read_call, a);
	
	// Creating the basicblock to check while loop condition
	// Creating load instructions. Do not use LLVMBuildLoad instead use LLVMBuildLoad2
	
  LLVMBasicBlockRef cmp_BB = LLVMAppendBasicBlock(func, "");
	LLVMBuildBr(builder, cmp_BB); // Before changing to new BB emit a branch
  LLVMPositionBuilderAtEnd(builder, cmp_BB);

	LLVMValueRef l1 = LLVMBuildLoad2(builder, LLVMInt32Type(), m, "");	
	LLVMValueRef l2 = LLVMBuildLoad2(builder, LLVMInt32Type(), n, "");	

	//Creating an arithmetic instruction. In this case it is add instruction
	LLVMValueRef t3 = LLVMBuildICmp(builder, LLVMIntSLT, l1, l2, "");

	//Creating basic blocks for while body and BB after while
	
  LLVMBasicBlockRef while_BB = LLVMAppendBasicBlock(func, "");
  LLVMBasicBlockRef final = LLVMAppendBasicBlock(func, "");
	
	
	LLVMBuildCondBr(builder, t3, while_BB, final);

	//Creating instuctions in body of while loop
  LLVMPositionBuilderAtEnd(builder, while_BB);
	
	LLVMValueRef t4 = LLVMBuildLoad2(builder, LLVMInt32Type(), m, "");
	LLVMValueRef const_10 = LLVMConstInt(LLVMInt32Type(), 10, 1);
	LLVMValueRef add_op = LLVMBuildAdd(builder, t4, const_10, "");
	LLVMBuildStore(builder, add_op, m);
	
	LLVMBuildBr(builder, cmp_BB);

	//Creating instructions in final basic block
	
  LLVMPositionBuilderAtEnd(builder, final);

	LLVMValueRef t6 = LLVMBuildLoad2(builder, LLVMInt32Type(), m, "");
  LLVMBuildRet(builder, t6);

	//Writing out the LLVM IR file and to stderr
	LLVMDumpModule(mod); //stderr
	LLVMPrintModuleToFile (mod, "test.ll", NULL); //to file here

	//Cleanup
  LLVMDisposeBuilder(builder);
	LLVMDisposeModule(mod);
}
