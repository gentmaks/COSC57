# COSC57 Compiler Workspace

This repository contains course work for COSC57 and now includes a **single source of truth Makefile** at the project root:

- `gent/COSC57/Makefile`

That root Makefile lets you build and run the compiler pipeline from one place, similar to `py/cosc057project-Compiler`.

## Requirements

Make sure these tools are installed and available on your `PATH`:

- `g++`
- `clang`
- `flex`
- `bison`
- `llvm-config-17` (or `llvm-config`)

## Main Build Targets

From `gent/COSC57`:

```bash
make compiler
make optimizer
make asmgen
```

- `make compiler` builds `frontend/minic`
- `make optimizer` builds `optimizations/optimizer`
- `make asmgen` builds `build/tools/asmgen`

You can also build everything at once:

```bash
make
```

## Running the Compiler Pipeline

The root Makefile uses:

- `IN` for the input miniC source file
- `build/output.ll` for raw LLVM IR
- `build/output_opt.ll` for optimized LLVM IR
- `build/output.s` for generated assembly

Default input:

```bash
frontend/test_mine_good.c
```

### 1) Run frontend compiler only

```bash
make run IN=frontend/test_mine_good.c
```

This runs `frontend/minic` and writes:

- `build/output.ll`

### 2) Run optimizer after frontend

```bash
make opt IN=frontend/test_mine_good.c
```

This writes:

- `build/output.ll`
- `build/output_opt.ll`

### 3) Run full pipeline through assembly

```bash
make asm IN=frontend/test_mine_good.c
```

or:

```bash
make pipeline IN=frontend/test_mine_good.c
```

This writes:

- `build/output.ll`
- `build/output_opt.ll`
- `build/output.s`

### 4) Build and run generated assembly (32-bit)

```bash
make run32 IN=frontend/test_mine_good.c
```

This links `build/output.s` with `optimizer_test_results/main.c`, builds:

- `build/output.out`

and executes it.

Note: `run32` needs a working x86 32-bit toolchain (`clang -m32`), which is commonly available on Linux x86 environments and may not work on macOS arm64.

### 5) Run on your host architecture (recommended on macOS arm64)

```bash
make run-native IN=frontend/test_mine_good.c
```

This compiles `build/output_opt.ll` together with `optimizer_test_results/main.c` and runs the result on the current machine architecture.

## Helpful Commands

Show available commands:

```bash
make help
```

Remove generated outputs:

```bash
make clean
```

Remove generated outputs and tool binaries:

```bash
make clean
make clean-tools
```
