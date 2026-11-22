#lang en
read(n)
read(v[0..n])
total = 0
for i=0 thr n - 1 {
    if v[i] > 0 and not (v[i] == 3) {
        total = total + v[i]
    }
}
write("total", total, sum(x * 2 for x in v))
