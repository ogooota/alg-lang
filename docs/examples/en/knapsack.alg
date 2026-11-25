#lang en
#decl best[w + 1];

read(n, w)
for i=0 to n {
    read(weight, value)
    for (cap = w; cap >= weight; cap--)
        best[cap] = max(best[cap], best[cap - weight] + value)
}
write(best[w])
