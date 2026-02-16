OPT = optimizer

LLVM_CONFIG := $(shell command -v llvm-config-17 2>/dev/null || command -v llvm-config 2>/dev/null)
ifeq ($(LLVM_CONFIG),)
$(error llvm-config not found. Install LLVM (e.g., `brew install llvm`) and ensure llvm-config is on PATH)
endif

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --ldflags --libs core irreader)

$(OPT): $(OPT).c optimizer.h
	g++ -x c++ -g $(LLVM_CFLAGS) -c $(OPT).c
	g++ $(OPT).o $(LLVM_LIBS) -o $@

clean:
	rm -f $(OPT) *.o
