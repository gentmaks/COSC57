# Optimizer

Build:

```bash
make
```

Usage:

```bash
./optimizer <input.ll> [output.ll]
```

If `output.ll` is omitted, the optimizer writes `optimized.ll`.

Alternatively can run the bash file which creates a directory with the generated `.ll` files and the diff files between those `.ll` files and the `.ll` files provided to us by prof. Kommineni

Usage: 
```bash
./run_optimizer_diffs.sh
```