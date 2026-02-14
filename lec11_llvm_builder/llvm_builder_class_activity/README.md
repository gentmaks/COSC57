# COSC 57, Gent Maksutaj

This project demonstrates generating LLVM IR using the LLVM C API.

## LLVM Version
This project was built and tested with:

```bash
llvm-config --version
21.1.8
```

## How to Build and Run

1. Clean previous builds:

```bash
make clean
```

2. Compile the LLVM builder

```bash
make
```

3. Run the builder to generate the LLVM IR for `class_test`:

```bash
./llvm_builder
```

4. View the generated LLVM IR:

```bash
cat class_test.ll
```
