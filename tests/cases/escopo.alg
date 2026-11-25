#lang en
#decl timer
timer = 0
total = 100
seen = {0, 0, 0}
fn visit(u) {
    timer++
    seen[u] = timer
    total = 5
    read(w)
    [a, b] = (1, 2)
    ret total + w + a
}
write(visit(1), visit(2), timer, seen, total)
