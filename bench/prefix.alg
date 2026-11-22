#lang en
#decl pre[n + 1];

read(n, a[0..n], q)
for i=0 to n {
    pre[i + 1] = pre[i] + a[i]
}
for k=0 to q {
    read(l, r)
    write(pre[r] - pre[l])
}
