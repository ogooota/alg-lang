#!/usr/bin/env python3
"""Compara alg (com e sem checagens) com C++ -O2: compilação, execução, memória e tamanho."""
import os, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ALG = os.path.join(HERE, "..", "alg")
OUT = os.path.join(HERE, "out")
PROBS = sys.argv[1:] or ["prefix", "knap", "sieve", "fib", "lcs", "sort", "dijkstra", "bfs", "mapa", "dsu", "fenwick", "segtree", "rmq", "primes"]
RUNS = 5


def timed(cmd):
    t = time.perf_counter()
    subprocess.run(cmd, check=True)
    return time.perf_counter() - t


def run(exe, inp):
    best, mem, out = 1e9, 0, None
    for _ in range(RUNS):
        with open(inp, "rb") as f:
            t = time.perf_counter()
            # /usr/bin/time mede o pico de memória do próprio programa (%M, em KB)
            p = subprocess.run(["/usr/bin/time", "-f", "%M", exe], stdin=f, capture_output=True)
            dt = time.perf_counter() - t
        if p.returncode != 0:
            sys.exit(f"{exe} falhou")
        kb = int(p.stderr.split()[-1])
        best, mem, out = min(best, dt), max(mem, kb), p.stdout
    return best, mem / 1024, out


def code_size(path):
    lines = [l for l in open(path) if l.strip()]
    return len(lines), sum(len(l.strip()) for l in lines)


os.makedirs(OUT, exist_ok=True)
print(f"{'problema':8} {'versão':9} {'compila':>8} {'roda':>8} {'x C++':>6} {'mem MB':>7}")
for p in PROBS:
    src, inp = os.path.join(HERE, p), os.path.join(HERE, p + ".in")
    exes = {
        "C++ -O2": (["g++", "-O2", "-std=c++17", "-o", f"{OUT}/{p}_cpp", src + ".cpp"], f"{OUT}/{p}_cpp"),
        "alg": ([ALG, "-o", f"{OUT}/{p}_alg", src + ".alg"], f"{OUT}/{p}_alg"),
        "alg -O": ([ALG, "-O", "-o", f"{OUT}/{p}_algO", src + ".alg"], f"{OUT}/{p}_algO"),
    }
    res = {}
    for name, (cc, exe) in exes.items():
        res[name] = (timed(cc),) + run(exe, inp)
    outs = {r[3] for r in res.values()}
    if len(outs) != 1:
        sys.exit(f"{p}: saídas diferentes!")
    base = res["C++ -O2"][1]
    for name, (ct, rt, mem, _) in res.items():
        print(f"{p:8} {name:9} {ct:7.2f}s {rt:7.3f}s {rt / base:5.2f}x {mem:7.1f}")
    la, ca = code_size(src + ".alg")
    lc, cc_ = code_size(src + ".cpp")
    print(f"{'':8} código: alg {la} linhas/{ca} chars · C++ {lc} linhas/{cc_} chars\n")
