LLVM_CONFIG := $(shell command -v llvm-config-17 2>/dev/null || command -v llvm-config 2>/dev/null)
ifeq ($(LLVM_CONFIG),)
$(error llvm-config not found. Install LLVM (e.g., `brew install llvm`) and ensure llvm-config is on PATH)
endif

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --ldflags --libs core)

minic: minic.y minic.l ast.c semantic.cpp ../LLVM_IR_BUILDER/ir_builder.cpp ../LLVM_IR_BUILDER/ir_builder.h semantic.h ast.h
	bison -d minic.y
	flex minic.l
	g++ -Wall -Wextra -g $(LLVM_CFLAGS) -o minic minic.tab.c lex.yy.c ast.c semantic.cpp ../LLVM_IR_BUILDER/ir_builder.cpp $(LLVM_LIBS)

clean:
	rm -f minic minic.tab.c minic.tab.h lex.yy.c
