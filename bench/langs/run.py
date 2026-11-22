#!/usr/bin/env python3
"""Mesmo problema em várias linguagens: compila, mede (intercalado) e confere que as saídas batem com o C++."""
import os, shutil, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.dirname(HERE)
OUT = os.path.join(BENCH, "out", "langs")
ALG = os.path.join(BENCH, "..", "alg")
PROBLEMS = sys.argv[1:] or ["sort", "prefix", "sieve", "knap", "fib", "dsu", "mapa", "dijkstra", "words"]
LIMIT = 60

# nome → (comando de compilação ou None, comando de execução)
def languages(p):
    exe = lambda tag: os.path.join(OUT, f"{p}_{tag}")
    return {
        "alg -O": ([ALG, "-O", "-o", exe("alg"), os.path.join(BENCH, p + ".alg")], [exe("alg")]),
        "C++ -O2": (["g++", "-O2", "-std=c++17", "-o", exe("cpp"), os.path.join(BENCH, p + ".cpp")], [exe("cpp")]),
        "C -O2": (["gcc", "-O2", "-o", exe("c"), os.path.join(HERE, p + ".c")], [exe("c")]),
        "Rust -O": (["rustc", "-C", "opt-level=3", "-o", exe("rs"), os.path.join(HERE, p + ".rs")], [exe("rs")]),
        "JS (Node)": (None, ["node", os.path.join(HERE, p + ".js")]),
        "Python": (None, ["python3", os.path.join(HERE, p + ".py")]),
    }


def run_once(command, input_path):
    with open(input_path, "rb") as f:
        start = time.perf_counter()
        try:
            result = subprocess.run(command, stdin=f, capture_output=True, timeout=LIMIT)
        except subprocess.TimeoutExpired:
            return None, None
        return time.perf_counter() - start, result.stdout


os.makedirs(OUT, exist_ok=True)
summary = {}
for p in PROBLEMS:
    langs = languages(p)
    for name, (compile_command, _) in langs.items():
        if compile_command:
            subprocess.run(compile_command, check=True, capture_output=True)
    input_path = os.path.join(BENCH, p + ".in")
    best, outputs, slow = {}, {}, set()
    for _ in range(3):
        for name, (_, command) in langs.items():
            if name in slow or (name in best and best[name] is None):
                continue
            elapsed, output = run_once(command, input_path)
            if elapsed is None:
                best[name] = None
                continue
            best[name] = min(best.get(name) or 9e9, elapsed)
            outputs[name] = output
            if elapsed > 3:
                slow.add(name)
    reference = outputs["C++ -O2"]
    base = best["C++ -O2"]
    print(f"\n{p}")
    for name in langs:
        t = best[name]
        if t is None:
            print(f"  {name:10} > {LIMIT}s")
            summary.setdefault(name, []).append(None)
            continue
        mark = "" if outputs[name] == reference else "  SAÍDA DIFERENTE"
        print(f"  {name:10} {t:7.3f}s  {t / base:6.2f}x C++{mark}")
        summary.setdefault(name, []).append(t / base)

print("\nmédia geométrica (x C++, menor é melhor):")
for name, ratios in summary.items():
    done = [r for r in ratios if r is not None]
    mean = 1.0
    for r in done:
        mean *= r
    mean **= 1 / len(done)
    missing = len(ratios) - len(done)
    print(f"  {name:10} {mean:6.2f}x" + (f"  ({missing} passou de {LIMIT}s, fora da média)" if missing else ""))
