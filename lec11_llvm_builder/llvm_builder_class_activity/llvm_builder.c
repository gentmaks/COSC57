#include <llvm-c/Core.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    // Step 1 -> Creating the module
    LLVMModuleRef mod = LLVMModuleCreateWithName("");
	//LLVMSetTarget(mod, "x86_64-pc-linux-gnu");

	//Creating extern function declaration. A function without a body is treated as declaration.
    LLVMTypeRef param_print[] = {LLVMInt32Type()};
    LLVMTypeRef ret_print = LLVMFunctionType(LLVMVoidType(), param_print, 1, 0);
    LLVMValueRef func_print = LLVMAddFunction(mod, "print", ret_print);

	//Creating extern function declaration. A function without a body is treated as declaration.
    LLVMTypeRef param_read[] = {};
    LLVMTypeRef ret_read = LLVMFunctionType(LLVMInt32Type(), param_read, 0, 0);
    LLVMValueRef func_read = LLVMAddFunction(mod, "read", ret_read);

    //Creating a function in a module
    LLVMTypeRef param_types[] = { LLVMInt32Type() };
    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), param_types, 1, 0);
    LLVMValueRef func = LLVMAddFunction(mod, "func", func_type);

    //Creating a basic block
    LLVMBasicBlockRef first = LLVMAppendBasicBlock(func, "");

    //All instructions need to be created using a builder. The builder specifies
    //where the instructions are added.
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, first);

    //Creating an alloc instruction with alignment
    LLVMValueRef p = LLVMBuildAlloca(builder, LLVMInt32Type(), "p"); 
    LLVMSetAlignment(p, 4);
	LLVMValueRef a = LLVMBuildAlloca(builder, LLVMInt32Type(), "a"); 
	LLVMSetAlignment(a, 4);

    // Creating store instruction to make local copy of param
	LLVMBuildStore(builder, LLVMGetParam(func, 0), p);

    //Creating a call to read
    LLVMValueRef *read_args = NULL;
    LLVMValueRef read_call = LLVMBuildCall2(builder, ret_read, func_read, read_args, 0, ""); 

    //Creating store instructions for local variables
    LLVMBuildStore(builder, read_call, a);

    // Dealing with the if-else blocks now
    // Creating load instructions for the conditional statement
	LLVMValueRef l1 = LLVMBuildLoad2(builder, LLVMInt32Type(), a, "");	
	LLVMValueRef l2 = LLVMBuildLoad2(builder, LLVMInt32Type(), p, "");	
    // Creating an arithmetic instruction for comparing
	LLVMValueRef t3 = LLVMBuildICmp(builder, LLVMIntSGT, l1, l2, "");

    // Creating the basic blocks for if and then
    // We won't be needing final since we return on both branches
    LLVMBasicBlockRef if_BB = LLVMAppendBasicBlock(func, "");
    LLVMBasicBlockRef else_BB = LLVMAppendBasicBlock(func, "");


    // Building the Conditional branch with if and else
    LLVMBuildCondBr(builder, t3, if_BB, else_BB);

    // If bblock returning p
    LLVMPositionBuilderAtEnd(builder, if_BB);
    LLVMValueRef t4 = LLVMBuildLoad2(builder, LLVMInt32Type(), p, "");
    LLVMBuildRet(builder, t4);

    // Else bblock returning a
    LLVMPositionBuilderAtEnd(builder, else_BB);
    LLVMValueRef t5 = LLVMBuildLoad2(builder, LLVMInt32Type(), a, "");
    LLVMBuildRet(builder, t5);

    //Writing out the LLVM IR
    LLVMDumpModule(mod);
    LLVMPrintModuleToFile (mod, "class_test.ll", NULL);

    //Cleanup
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);

}
