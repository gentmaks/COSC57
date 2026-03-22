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

For assembly backend programs, use files in:

```bash
assembly_gen_tests/
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

## Running `assembly_gen_tests` Programs

Use a single `.c` file for `IN` (not a directory):

```bash
make run32 IN=assembly_gen_tests/square.c
make run32 IN=assembly_gen_tests/sum_n.c
```

Valid form:

```bash
make run32 IN=assembly_gen_tests/<file>.c
```

Invalid form (directory path):

```bash
make run32 IN=assembly_gen_tests/
```

### Recommended flow on Linux server

Rebuild tool binaries on the server before running tests:

```bash
make clean-tools
make compiler optimizer asmgen
make run32 IN=assembly_gen_tests/square.c
```

This avoids cross-platform binary issues (for example, trying to run a macOS-built `frontend/minic` on Linux).

## Common Parse Errors in `assembly_gen_tests`

The frontend parser implements a restricted miniC grammar. Some C constructs are intentionally not accepted.

- Extra semicolon after a statement can fail parse (`prod = 1;;` in `assembly_gen_tests/fact.c` should be `prod = 1;`)
- Declaration with initializer is not supported (`int max = 0;` in `assembly_gen_tests/max_n.c` should be split into `int max;` then `max = 0;`)

If parsing fails, check the input program against the grammar used by `frontend/minic.y`.

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
