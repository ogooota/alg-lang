#lang en
n = 5
v = {}
for (i = 0; i < n; i++) {
    if (i == 2) continue
    v.add(i * 10)
}
write(v, i)
for (j = n - 1; j >= 0; j--) write(j)
for (k = 1; k < 100; k *= 3)
    write(k)
for (x in v) write(x)
for (i = 0 to 3) write(i)
for (i = 0 thr 3) { write(i) }
m = map {}
m[1] = 2
for ([a, b] in m) write(a, b)
for [a, b] in m
    write(a, b)
t = 0
while (t < 3) t++
++t
t--
--t
write(t)
if (t == 2) write("two") else write("no")
if (t > 5) {
    write("big")
} else if (t > 1) {
    write("mid")
}
c = 0
for (;;) {
    c += 1
    if (c > 4) break
}
write(c)
w = {1, 2, 3}
w[1]++
write(w)
n = 5
v = {1}
for (i = 0; i < n; i++) {
    if (i == 1) n = 3
}
write(i, n)
for (i = 0; i < v.size(); i++) {
    if (v.size() < 4) v.add(i)
}
write(v)
for (i = 0; i < 10; i++) {
    i += 2
}
write(i)
read(k)
for (i = 0; i < k; i++) read(k)
write(i, k)
