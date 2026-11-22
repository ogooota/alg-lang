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
| 2 | C -O2 | 1.31× |
| 3 | Rust (opt-level 3) | 1.34× |
| 4 | C++ -O2 | 2.00× |
| 5 | JavaScript (Node 24) | 3.33× |
| 6 | Python 3.12 | 20.3× |

Per problem (alg's own time, then the time of each language relative to alg; below 1.00× means faster than alg):

| problem | alg -O | C | Rust | C++ | JS | Python |
|---|---|---|---|---|---|---|
| sort 10⁶ integers | 0.057 s | 3.44× | 1.30× | 3.28× | 7.95× | 22.1× |
| prefix sums, 10⁶ queries | 0.204 s | 1.19× | 1.06× | 2.25× | 2.85× | 8.70× |
| sieve up to 2·10⁷ | 0.182 s | 0.96× | 0.98× | 0.99× | 1.54× | 10.8× |
| 0/1 knapsack | 0.042 s | 1.79× | 3.50× | 1.81× | 5.69× | 594× |
| recursive fib(38) | 0.094 s | 1.06× | 1.31× | 0.98× | 6.82× | 70.1× |
| union-find, 10⁶ operations | 0.144 s | 1.17× | 1.06× | 2.14× | 2.03× | 14.6× |
| count 10⁶ values with a map | 0.110 s | 1.01× | 0.98× | 2.59× | 2.50× | 4.21× |
| Dijkstra, 10⁵ vertices, 10⁶ edges | 0.144 s | 1.10× | 2.01× | 3.18× | 1.85× | 21.8× |
| count 10⁶ words | 0.265 s | 1.15× | 1.11× | 2.29× | 3.96× | 5.00× |

The sieve row is the same hand-written loop in every language. alg's built-in `sieve(n)` (segmented, mod-30 wheel, after primesieve) lists the primes up to 2·10⁷ in 0.021 s, about 10× faster than that loop.

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
