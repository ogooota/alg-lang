#lang en
read(n, v[0..n])
sort(v)
total = 0
for i=0 to n {
    total = (total + v[i] % 1000003 * (i % 1000)) % 1000000007
}
write(total)
