#lang en
#decl best[w + 1];

read(n, w)
for i=0 to n {
    read(weight, value)
    for c=0 thr w - weight {
        cap = w - c
        best[cap] = max(best[cap], best[cap - weight] + value)
    }
}
write(best[w])
