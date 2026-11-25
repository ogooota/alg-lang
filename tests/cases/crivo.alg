#lang en
// crivo nativo: limites pequenos, bordas de segmento (983040) e do padrão (510510)
read(q)
for t=0 to q {
    read(n)
    p = sieve(n)
    h = 0
    for x in p
        h = (h * 31 + x) % 1000000007
    write(p.size(), h)
}
