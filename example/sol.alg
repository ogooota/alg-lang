#lang en 
#decl {
    pre[n+1];
}

read(n, l, r, a[0..n])
for i=0 to n {
    pre[i+1]=pre[i]+a[i]
}

write(pre[r]-pre[l-1])
