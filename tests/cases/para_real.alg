#lang en
n = 3
for (x = 0.5; x < n; x++) write(x)
for (y = 0.5; y < 2.5; y += 0.5) write(y)
fn f(a) {
    s = 0.0
    for (x = a; x < 3; x++) s += x
    ret s
}
write(f(1), f(0.5))
t = 0.5
for (t = 0; t < 2; t++) write(t)
[a, b] = (1, 2)
[a, b] = (b, a)
write(a, b)
