LLVMCODE = llvm_builder
TEST = class_test

LLVM_CONFIG = llvm-config
CFLAGS = `$(LLVM_CONFIG) --cflags`
LDFLAGS = `$(LLVM_CONFIG) --ldflags --libs core`

$(LLVMCODE): $(LLVMCODE).c
	clang -g $(CFLAGS) -c $(LLVMCODE).c
	clang $(LLVMCODE).o $(LDFLAGS) -o $@

minic: $(LLVMCODE)
	./$(LLVMCODE)
	clang main.c $(TEST).ll -o $(TEST).out

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).s

clean:
	rm -f $(TEST).ll
	rm -f $(LLVMCODE)
	rm -f *.o
	rm -f *.out
