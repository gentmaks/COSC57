LLVM_CONFIG := $(shell command -v llvm-config-17 2>/dev/null || command -v llvm-config 2>/dev/null)
ifeq ($(LLVM_CONFIG),)
$(error llvm-config not found. Install LLVM (e.g., `brew install llvm`) and ensure llvm-config is on PATH)
endif

LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LIBS_CORE_IR := $(shell $(LLVM_CONFIG) --ldflags --libs core irreader)

FRONTEND_DIR := FRONTEND
OPT_DIR := OPTIMIZATIONS
ASM_DIR := ASSEMBLY_CODE_GENERATION

MINIC := $(FRONTEND_DIR)/minic
OPTIMIZER := $(OPT_DIR)/optimizer

INPUT ?= assembly_gen_tests/fact.c
INPUT_ABS := $(abspath $(INPUT))
CASE := $(basename $(notdir $(INPUT)))

OUT_DIR := build/pipeline
ASMGEN_HOST := $(OUT_DIR)/asmgen
RAW_LL := $(OUT_DIR)/$(CASE).raw.ll
OPT_LL := $(OUT_DIR)/$(CASE).opt.ll
ASM_S := $(OUT_DIR)/$(CASE).s
RUN_EXE := $(OUT_DIR)/$(CASE).out

.PHONY: help frontend optimize assembly pipeline run32 clean clean-tools

help:
	@echo "Compiler pipeline targets:"
	@echo "  make pipeline INPUT=<path/to/file.c>   # frontend -> optimizer -> assembly"
	@echo "  make frontend INPUT=<path/to/file.c>   # lexical/syntax/semantic + raw LLVM IR"
	@echo "  make optimize INPUT=<path/to/file.c>   # raw LLVM IR + optimized LLVM IR"
	@echo "  make assembly INPUT=<path/to/file.c>   # optimized LLVM IR + assembly"
	@echo "  make run32 INPUT=<path/to/file.c>      # Linux-style 32-bit run (if toolchain supports)"
	@echo "  make clean                             # remove generated pipeline outputs"
	@echo "  make clean-tools                       # remove built binaries in subprojects"

$(OUT_DIR):
	mkdir -p "$(OUT_DIR)"

$(MINIC):
	$(MAKE) -C "$(FRONTEND_DIR)"

$(OPTIMIZER):
	$(MAKE) -C "$(OPT_DIR)"

$(ASMGEN_HOST): $(ASM_DIR)/assembly_generator.c $(ASM_DIR)/assembly_generator.h | $(OUT_DIR)
	g++ -Wall -Wextra -g -x c++ $(LLVM_CFLAGS) -o "$(ASMGEN_HOST)" "$(ASM_DIR)/assembly_generator.c" $(LLVM_LIBS_CORE_IR)

$(RAW_LL): $(MINIC) $(OUT_DIR)
	cd "$(FRONTEND_DIR)" && ./minic < "$(INPUT_ABS)"
	cp "$(FRONTEND_DIR)/minic.ll" "$(RAW_LL)"

$(OPT_LL): $(RAW_LL) $(OPTIMIZER)
	"$(OPTIMIZER)" "$(RAW_LL)" "$(OPT_LL)"

$(ASM_S): $(OPT_LL) $(ASMGEN_HOST)
	"$(ASMGEN_HOST)" "$(OPT_LL)" "$(ASM_S)"

frontend: $(RAW_LL)
	@echo "Raw LLVM IR: $(RAW_LL)"

optimize: $(OPT_LL)
	@echo "Optimized LLVM IR: $(OPT_LL)"

assembly: $(ASM_S)
	@echo "Assembly output: $(ASM_S)"

pipeline: $(ASM_S)
	@echo "Pipeline complete for $(INPUT)"
	@echo "Raw LLVM IR      -> $(RAW_LL)"
	@echo "Optimized LLVM IR-> $(OPT_LL)"
	@echo "Assembly         -> $(ASM_S)"

run32: $(ASM_S)
	clang -m32 "$(ASM_S)" "assembly_gen_tests/main.c" -o "$(RUN_EXE)"
	"$(RUN_EXE)"

clean:
	rm -rf "$(OUT_DIR)"

clean-tools:
	$(MAKE) -C "$(FRONTEND_DIR)" clean
	$(MAKE) -C "$(OPT_DIR)" clean
