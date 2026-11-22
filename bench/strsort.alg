#lang en
// sort words in dictionary order
#decl w: str
read(n, w[0..n])
sort(w)
for i=0 to n
    write(w[i])
