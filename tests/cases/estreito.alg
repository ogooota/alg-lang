#lang pt
// cada vetor aqui testa uma regra da análise de faixa (range.c)
#decl {
    mods[10];
    bits[10];
    grande[3];
    neg[4];
    cont[300];
}

para i=0 ate 10 {
    mods[i] = (i * 1000003) % 1000000007
    bits[i] = i & 3
}
escrever(mods)
escrever(bits)

// cresce sem limite provado: precisa ficar com 64 bits
para i=0 ate 3 {
    grande[i] = 1
}
para k=0 ate 70 {
    grande[k % 3] = grande[k % 3] * 2 + 1
}
escrever(grande)

// negativos pequenos
para i=0 ate 4 {
    neg[i] = 0 - i * 30
}
escrever(neg)

// contador que passa de 255 (não pode virar 1 byte)
para i=0 ate 1000 {
    cont[i % 300] = cont[i % 300] + 1
}
escrever(cont[0], cont[299])

// mesma função recebendo dois vetores: o segundo recebe valores grandes
fn enche(w, valor) {
    para i=0 ate tam(w) {
        w[i] = valor
    }
}

p = {1, 2, 3}
q = {1, 2, 3}
enche(p, 7)
enche(q, 5000000000)
escrever(p, q)

// lido da entrada: 64 bits
ler(x[0..2])
escrever(x)
