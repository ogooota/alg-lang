#lang en
cont = map {}
read(n)
for i=0 to n {
    read(x)
    cont[x] = cont[x] + 1
}
melhor = 0
for [k, c] in cont {
    melhor = max(melhor, c)
}
write(cont.size(), melhor)
