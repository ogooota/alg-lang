#lang en
#decl composto[n + 1];

read(n)
total = 0
for i=2 thr n {
    if not composto[i] {
        total = total + 1
        j = i * i
        while j <= n {
            composto[j] = 1
            j = j + i
        }
    }
}
write(total)
