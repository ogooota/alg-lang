#!/usr/bin/env python3
"""Gera as entradas do benchmark (bench/*.in) com as mesmas sementes usadas nas medições."""
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))


def write(name, text):
    with open(os.path.join(HERE, name), "w") as f:
        f.write(text)


r = random.Random(42)
n = q = 10**6
pairs = (sorted((r.randint(0, n), r.randint(0, n))) for _ in range(q))
write("prefix.in", f"{n}\n" + " ".join(str(r.randint(-10**9, 10**9)) for _ in range(n)) + f"\n{q}\n"
      + "\n".join(f"{l} {h}" for l, h in pairs) + "\n")
n, cap = 1000, 100000
write("knap.in", f"{n} {cap}\n" + " ".join(str(r.randint(1, 1000)) for _ in range(n)) + "\n"
      + " ".join(str(r.randint(1, 10**6)) for _ in range(n)) + "\n")
write("sieve.in", "20000000\n")
write("fib.in", "38\n")
n = m = 4000
write("lcs.in", f"{n} {m}\n" + " ".join(str(r.randint(1, 20)) for _ in range(n)) + "\n"
      + " ".join(str(r.randint(1, 20)) for _ in range(m)) + "\n")

r = random.Random(7)
n = 10**6
write("sort.in", f"{n}\n" + " ".join(str(r.randint(0, 10**18)) for _ in range(n)) + "\n")
n, m = 10**5, 10**6
write("dijkstra.in", f"{n} {m}\n" + "\n".join(
    f"{r.randrange(n)} {r.randrange(n)} {r.randint(1, 10**6)}" for _ in range(m)) + "\n")
n, m = 10**6, 2 * 10**6
write("bfs.in", f"{n} {m}\n" + "\n".join(f"{r.randrange(n)} {r.randrange(n)}" for _ in range(m)) + "\n")
n = 10**6
write("mapa.in", f"{n}\n" + " ".join(str(r.randint(0, 3 * 10**5)) for _ in range(n)) + "\n")
print("entradas geradas em", HERE)

# estruturas nativas (técnicas)
r = random.Random(11)
n = q = 10**6
write("dsu.in", f"{n} {q}\n" + "\n".join(
    f"{r.randint(1, 2)} {r.randrange(n)} {r.randrange(n)}" for _ in range(q)) + "\n")


def updates_and_ranges(r, n, q, kind_first, value_max):
    lines = []
    for _ in range(q):
        if r.random() < 0.5:
            lines.append(f"1 {r.randrange(n)} {r.randint(0, value_max)}")
        else:
            low = r.randrange(n)
            high = r.randint(low + 1, n)
            lines.append(f"2 {low} {high}")
    return "\n".join(lines)


write("fenwick.in", f"{n} {q}\n" + updates_and_ranges(r, n, q, 1, 1000) + "\n")
values = " ".join(str(r.randint(0, 10**9)) for _ in range(n))
write("segtree.in", f"{n} {q}\n{values}\n" + updates_and_ranges(r, n, q, 1, 10**9) + "\n")
ranges = []
for _ in range(q):
    low = r.randrange(n)
    ranges.append(f"{low} {r.randint(low + 1, n)}")
write("rmq.in", f"{n} {q}\n{values}\n" + "\n".join(ranges) + "\n")
write("primes.in", "20000000\n")
print("entradas das estruturas geradas")

# palavras: 10^6 palavras de 1 a 8 letras (muitas repetidas) para contar frequência e ordenar
r = random.Random(13)
n = 10**6
words = ("".join(r.choice("abcdefgh") for _ in range(r.randint(1, 8))) for _ in range(n))
write("words.in", f"{n}\n" + " ".join(words) + "\n")
write("strsort.in", open(os.path.join(HERE, "words.in")).read())
