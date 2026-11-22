#lang en
// percorrer mapa: ordem, valores vivos quando o corpo escreve, chaves tupla e negativas
m = map {}
for i=0 to 8
    m[(i * 37) % 11 - 5] += i
for (k, v) in m
    write(k, v)

// o corpo muda valores de outras chaves: as próximas iterações veem o valor novo
for (k, v) in m {
    write("vendo", k, v)
    m[k + 1] += 100
}

// remover durante o laço não muda as chaves percorridas
for (k, v) in m
    m.remove(k + 1)
write(len(m))

t = map {}
t[(2, 1)] = 5
t[(1, 9)] = 7
t[(1, 2)] = 3
for (pair, amount) in t
    write(pair, amount)

// muitas strings com o mesmo prefixo de 8 bytes: desempate grande
s = map {}
for i=0 to 40
    s["prefixo_" + str((i * 17) % 40)] += i
for (name, total) in s
    write(name, total)
