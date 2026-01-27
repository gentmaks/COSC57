LLVMCODE = llvm_parser
TEST = class_minic

LLVM_CONFIG := $(shell command -v llvm-config-17 2>/dev/null || command -v llvm-config 2>/dev/null)
ifeq ($(LLVM_CONFIG),)
$(error llvm-config not found. Install LLVM (e.g., `brew install llvm`) and ensure llvm-config is on PATH)
endif

LLVM_INC := $(shell $(LLVM_CONFIG) --includedir)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --cxxflags --ldflags --libs core)

$(LLVMCODE): $(LLVMCODE).c
	gcc -g -I $(LLVM_INC) -c $(LLVMCODE).c
	g++ $(LLVMCODE).o $(LLVM_LIBS) -o $@

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
