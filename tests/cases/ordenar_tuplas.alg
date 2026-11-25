#lang en
#decl seed
seed = 12345
fn rnd(m) -> int {
    seed = (seed * 1103515245 + 12345) % 2147483648
    ret seed % m
}
fn check(v, w, name) {
    bad = 0
    for (i = 1; i < v.size(); i++)
        if (v[i] < v[i - 1]) bad++
    n = w.size()
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (w[j] < w[i]) [w[i], w[j]] = (w[j], w[i])
    for (i = 0; i < n; i++)
        if (not (v[i] == w[i])) bad++
    write(name, bad)
}
for t=0 to 3 {
    n = 260 + rnd(300)
    m = 1 + rnd(5) * rnd(1000)
    a = {}
    b = {}
    c = {}
    d = {}
    for i=0 to n {
        a.add((rnd(m) - m / 2, rnd(3)))
        b.add((real(rnd(m)) / 7 - 30, rnd(4)))
        c.add((str(rnd(m)) + "x", rnd(2), rnd(9)))
        d.add(((rnd(3), rnd(m)), rnd(5)))
    }
    a2 = copy(a)
    b2 = copy(b)
    c2 = copy(c)
    d2 = copy(d)
    sort(a)
    sort(b)
    sort(c)
    sort(d)
    check(a, a2, "int")
    check(b, b2, "real")
    check(c, c2, "str")
    check(d, d2, "nested")
}
