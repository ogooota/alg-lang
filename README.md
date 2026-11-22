# alg

**Write the algorithm, not the boilerplate.** alg is a small language for competitive programming
and for studying algorithms. It reads like textbook pseudocode, compiles to C, and in our
benchmarks runs as fast as or faster than idiomatic C++.

```
#lang en
#decl dist[n];

read(n, m)
g = graph(n, directed)
for i=0 to m {
    read(u, v, w)
    g.edge(u - 1, v - 1, w)
}
for i=0 to n {
    dist[i] = INF
}
dist[0] = 0
h = heap()
h.push((0, 0))
while len(h) > 0 {
    (d, u) = h.pop()
    if d > dist[u] {
        continue
    }
    for (v, w) in g.neighbors(u) {
        if d + w < dist[v] {
            dist[v] = d + w
            h.push((dist[v], v))
        }
    }
}
write(dist)
```

That is a full Dijkstra: 27 lines against 32 in C++, with no `priority_queue<pair<long long,int>, vector<...>, greater<>>`,
no `ios::sync_with_stdio(false)` and no `long long` anywhere.

## Why alg

- **Less to write.** Reading an array is `read(n, a[0..n])`. Printing is `write(a, b, v)`. Every integer is 64-bit.
  Tuples, sorting, heaps, queues, maps and graphs are built in, and so are contest techniques: prefix sums,
  binary search, gcd/lcm/powmod, a prime sieve, union-find, Fenwick tree, segment tree and sparse table.
- **Fewer classic bugs.** `-7 % 2` is `1` (never negative), `INF + w` does not overflow, `x & 1 == 0` means what it says,
  recursion gets a 256 MB stack, undirected graphs add both edges, and the map uses a random hash seed per run.
- **Fast.** Fast I/O, radix sort, a compact (CSR) graph layout, open-addressing maps, and a range analysis that stores each
  array in 1, 2, 4 or 8 bytes depending on the values it can hold.
- **Safe while practicing, fast when submitting.** Without `-O`, an out-of-bounds index stops with the line number.
  With `-O`, the checks are gone. `--emit-c` prints a single C file you can submit to judges that accept C.
- **Your language.** Keywords exist in English (`#lang en`) and Portuguese (`#lang pt`); more languages are planned.

## Speed

alg with `-O` against C++ `-O2` (with `sync_with_stdio(false)`), same input, best of 5 runs. Lower is better.

| Problem | alg / C++ time |
|---|---|
| Dijkstra, 10⁵ vertices, 10⁶ edges | 0.36× |
| BFS, 10⁶ vertices, 2·10⁶ edges | 0.45× |
| sort 10⁶ integers | 0.46× |
| prefix sums, 10⁶ queries | 0.50× |
| 0/1 knapsack, 10⁸ operations | 0.57× |
| count 10⁶ values with a map | 0.62× |
| LCS 4000 × 4000 | 0.96× |
| sieve up to 2·10⁷ | 0.99× |
| recursive fib(38) | 1.01× |
| union-find, 10⁶ operations | 0.50× |
| Fenwick tree, 10⁶ updates and sums | 0.53× |
| segment tree (min), 10⁶ updates and queries | 0.56× |
| sparse table, 10⁶ range minimums | 0.57× |
| sort 10⁶ words | 0.81× |
| count 10⁶ words with a map | 0.84× |

Part of the gap is input speed. Against C++ with a hand-written fast reader, alg still comes out ahead on sorting (0.74×),
Dijkstra (0.62×), BFS (0.52×) and maps (0.86×). Reproduce with `python3 bench/gen_inputs.py && python3 bench/run.py`.

### Ranking across languages

The same nine problems in each language. Each number says how many times longer than alg the language took (geometric mean over the nine problems), so alg is 1.00× and higher means slower:

| # | language | time vs alg |
|---|---|---|
| 1 | **alg -O** | **1.00×** |
| 2 | C -O2 | 1.22× |
| 3 | Rust (opt-level 3) | 1.27× |
| 4 | C++ -O2 | 1.85× |
| 5 | JavaScript (Node 24) | 3.09× |
| 6 | Python 3.12 | 19.7× |

Per problem (time vs alg; below 1.00× means faster than alg):

| problem | C | Rust | C++ | JS | Python |
|---|---|---|---|---|---|
| sort 10⁶ integers | 3.53× | 1.35× | 3.38× | 8.20× | 23.6× |
| prefix sums, 10⁶ queries | 1.48× | 1.25× | 2.44× | 3.34× | 10.4× |
| sieve up to 2·10⁷ | 0.95× | 0.97× | 0.98× | 1.48× | 10.6× |
| 0/1 knapsack | 1.77× | 3.51× | 1.79× | 5.65× | 619× |
| recursive fib(38) | 1.08× | 1.30× | 0.97× | 6.75× | 78.4× |
| union-find, 10⁶ operations | 1.13× | 1.00× | 1.93× | 1.89× | 13.3× |
| count 10⁶ values with a map | 0.91× | 0.97× | 2.50× | 2.39× | 4.24× |
| Dijkstra, 10⁵ vertices, 10⁶ edges | 1.18× | 2.23× | 3.46× | 1.95× | 26.6× |
| count 10⁶ words | 0.53× | 0.52× | 1.08× | 1.89× | 2.33× |

The C, Rust and JS versions use a fast input reader. The C++ versions use plain `cin` with `sync_with_stdio(false)`.

> **Run it on your own machine.** These numbers come from one laptop: an Intel i5-7200U on WSL2, with gcc 13, rustc 1.99, Node 24 and Python 3.12. Your CPU, compiler versions and OS will change them. Reproduce with `python3 bench/gen_inputs.py && python3 bench/langs/run.py`; the script checks that every language prints the same answer.

## Getting started

Requirements: a C compiler (`cc`/gcc), `make` and `xxd`.

```sh
git clone https://github.com/ogooota/alg-lang && cd alg-lang
make install                 # builds alg and installs it to ~/.local/bin (PREFIX=/usr/local for system-wide)
alg prog.alg < input.txt     # compile and run
alg -O -o prog prog.alg      # optimized executable
make test                    # run the test suite
```

## Learn more

- **Guide** with seven contest problems side by side with C++: [`docs/guia.html`](docs/guia.html) (English, with a Portuguese version)
- **Specification** and feature status: [`docs/linguagem.md`](docs/linguagem.md) (Portuguese)
- **How the compiler works**: [`docs/compilador.md`](docs/compilador.md)
- **Editor support**: syntax highlighting, indentation and a color theme for Neovim in [`editors/nvim/`](editors/nvim/)

## Status

alg is young. Sets, records and step/descending loops are designed but not implemented yet;
runtime error messages are in Portuguese for now. Issues and ideas are welcome.
