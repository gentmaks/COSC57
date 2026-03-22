# COSC57 Compiler Workspace


Clone the directory using `git clone`

- The top level MAKEFILE serves as a single source of truth for the whole project.

## Requirements

Make sure these tools are installed and available on your `PATH`:

- `g++`
- `clang`
- `flex`
- `bison`
- `llvm-config-17` (or `llvm-config`)

## Main Build Targets

From `your_root_directory/COSC57`:

```bash
make compiler
make optimizer
make asmgen
```

- `make compiler` builds `frontend/minic`
- `make optimizer` builds `optimizations/optimizer`
- `make asmgen` builds `build/tools/asmgen`

(RECOMMENDED) You can also build everything at once:

```bash
make
```

## Running the Compiler Pipeline

The root Makefile uses:

- `IN` for the input miniC source file
- `MAIN` for the runtime driver file (default: `assembly_gen_tests/main.c`)
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

This links `build/output.s` with `assembly_gen_tests/main.c` (or your `MAIN=...` override), builds:

- `build/output.out`

and executes it.

Note: `run32` needs a working x86 32-bit toolchain (`clang -m32`), which is commonly available on Linux x86 environments and may not work on macOS arm64.

For host-native execution, you can also use:

```bash
make run-native IN=assembly_gen_tests/square.c
```

This target also uses `MAIN`, with the same default (`assembly_gen_tests/main.c`).

## Running `assembly_gen_tests` Programs

Use a single `.c` file for `IN` (not a directory):

```bash
make run32 IN=assembly_gen_tests/square.c
make run32 IN=assembly_gen_tests/sum_n.c
```

To use a different driver file:

```bash
make run32 IN=assembly_gen_tests/square.c MAIN=optimizer_test_results/main.c
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


## Parse errors in `assembly_gen_tests`

The frontend parser implements a restricted miniC grammar. Some C constructs are intentionally not accepted.

- Extra semicolon after a statement can fail parse (`prod = 1;;` in `assembly_gen_tests/fact.c` should be `prod = 1;`)
- Declaration with initializer is not supported (`int max = 0;` in `assembly_gen_tests/max_n.c` should be split into `int max;` then `max = 0;`)

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
