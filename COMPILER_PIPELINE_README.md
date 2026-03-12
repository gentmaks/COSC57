# Compiler Pipeline (End-to-End)

This project already has separate components for:

1. Frontend (lexical/syntax/semantic analysis + AST -> LLVM IR)
2. LLVM IR optimizations
3. Assembly generation

The root `Makefile` added here connects all of them so you can run one `.c` file through every stage.

## Prerequisites

- `llvm-config` on your `PATH`
- `clang` / `g++`
- Existing project subdirectories:
  - `FRONTEND/`
  - `OPTIMIZATIONS/`
  - `ASSEMBLY_CODE_GENERATION/`

## One-command full pipeline

From repo root:

```bash
make pipeline INPUT=FRONTEND/parser_tests/p1.c
```

This will:

- build `FRONTEND/minic` (if needed)
- run lexical + syntax + semantic analysis while generating raw LLVM IR
- build and run `OPTIMIZATIONS/optimizer`
- build and run `ASSEMBLY_CODE_GENERATION/asmgen`
- write outputs to `build/pipeline/`

## Stage-by-stage targets

```bash
make frontend INPUT=FRONTEND/parser_tests/p1.c
make optimize INPUT=FRONTEND/parser_tests/p1.c
make assembly INPUT=FRONTEND/parser_tests/p1.c
```

Generated files:

- raw LLVM IR: `build/pipeline/<name>.raw.ll`
- optimized LLVM IR: `build/pipeline/<name>.opt.ll`
- assembly: `build/pipeline/<name>.s`

## Optional 32-bit run

If your machine supports Linux-style `-m32` linking:

```bash
make run32 INPUT=FRONTEND/parser_tests/p1.c
```

On macOS (especially Apple Silicon), native `-m32` often fails due to assembler/ABI mismatch.
Use the Docker helper instead:

```bash
cd ASSEMBLY_CODE_GENERATION
./run_tests_docker.sh
```

## Cleanup

```bash
make clean
make clean-tools
```

## Input language note

`INPUT` must follow the MiniC grammar in `FRONTEND/minic.y` (the class subset of C).
Some files in `assembly_gen_tests/` may require small normalization (for example extra semicolons)
before the frontend parser accepts them.

