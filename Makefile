LLVM_CONFIG := $(shell command -v llvm-config-17 2>/dev/null || command -v llvm-config 2>/dev/null)
ifeq ($(LLVM_CONFIG),)
$(error llvm-config not found. Install LLVM (for example: brew install llvm) and ensure llvm-config is on PATH)
endif

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS_CORE_IR := $(shell $(LLVM_CONFIG) --ldflags --libs core irreader)

FRONTEND_DIR := frontend
OPT_DIR := optimizations
ASM_DIR := ASSEMBLY_CODE_GENERATION

COMPILER := $(FRONTEND_DIR)/minic
OPTIMIZER := $(OPT_DIR)/optimizer
ASMGEN := build/tools/asmgen

IN ?= frontend/test_mine_good.c
IN_ABS := $(abspath $(IN))

BUILD_DIR := build
OUT := $(BUILD_DIR)/output.ll
OUT_OPT := $(BUILD_DIR)/output_opt.ll
OUT_S := $(BUILD_DIR)/output.s
RUN_EXE := $(BUILD_DIR)/output.out
MAIN ?= assembly_gen_tests/main.c

.PHONY: all help compiler optimizer asmgen run opt asm pipeline run-native run32 clean clean-tools

all: compiler optimizer asmgen

help:
	@echo "Single source of truth Make targets:"
	@echo "  make compiler                         # build the frontend compiler (frontend/minic)"
	@echo "  make optimizer                        # build the optimizer binary"
	@echo "  make asmgen                           # build the assembly generator binary"
	@echo "  make run IN=<path/to/input.c>         # run compiler; emits $(OUT)"
	@echo "  make opt IN=<path/to/input.c>         # run compiler + optimizer; emits $(OUT_OPT)"
	@echo "  make asm IN=<path/to/input.c>         # run compiler + optimizer + asm generator; emits $(OUT_S)"
	@echo "  make pipeline IN=<path/to/input.c>    # alias of asm"
	@echo "  make run-native IN=<path/to/input.c> [MAIN=<path/to/main.c>]  # run optimized LLVM IR on host"
	@echo "  make run32 IN=<path/to/input.c> [MAIN=<path/to/main.c>]       # run generated 32-bit assembly"
	@echo "  make clean                            # remove generated outputs"
	@echo "  make clean-tools                      # clean binaries in subprojects"

$(BUILD_DIR):
	mkdir -p "$(BUILD_DIR)"

$(BUILD_DIR)/tools:
	mkdir -p "$(BUILD_DIR)/tools"

$(COMPILER):
	$(MAKE) -C "$(FRONTEND_DIR)"

$(OPTIMIZER):
	$(MAKE) -C "$(OPT_DIR)"

$(ASMGEN): $(ASM_DIR)/assembly_generator.c $(ASM_DIR)/assembly_generator.h | $(BUILD_DIR)/tools
	g++ -Wall -Wextra -g -x c++ $(LLVM_CFLAGS) -o "$(ASMGEN)" "$(ASM_DIR)/assembly_generator.c" $(LLVM_LIBS_CORE_IR)

compiler: $(COMPILER)

optimizer: $(OPTIMIZER)

asmgen: $(ASMGEN)

run: $(COMPILER) | $(BUILD_DIR)
	cd "$(FRONTEND_DIR)" && ./minic < "$(IN_ABS)"
	cp "$(FRONTEND_DIR)/minic.ll" "$(OUT)"
	@echo "Generated LLVM IR: $(OUT)"

opt: run $(OPTIMIZER)
	"$(OPTIMIZER)" "$(OUT)" "$(OUT_OPT)"
	@echo "Generated optimized LLVM IR: $(OUT_OPT)"

asm: opt $(ASMGEN)
	"$(ASMGEN)" "$(OUT_OPT)" "$(OUT_S)"
	@echo "Generated assembly: $(OUT_S)"

pipeline: asm

run-native: opt
	clang "$(OUT_OPT)" "$(MAIN)" -o "$(RUN_EXE)"
	"$(RUN_EXE)"

run32: asm
	clang -m32 "$(OUT_S)" "$(MAIN)" -o "$(RUN_EXE)"
	"$(RUN_EXE)"

clean:
	rm -rf "$(BUILD_DIR)"

clean-tools:
	$(MAKE) -C "$(FRONTEND_DIR)" clean
	$(MAKE) -C "$(OPT_DIR)" clean
