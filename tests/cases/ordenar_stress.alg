#lang pt
// compara ordenar() com inserção, para cada tamanho de elemento e de vetor
#decl semente
semente = 12345

fn proximo() {
    semente = (semente * 1103515245 + 12345) % 2147483648
    ret semente
}

fn insercao(v) {
    para i=1 ate v.tam() {
        x = v[i]
        j = i - 1
        enquanto j >= 0 e v[j] > x {
            v[j + 1] = v[j]
            j = j - 1
        }
        v[j + 1] = x
    }
}

fn iguais(a, b) {
    para i=0 ate a.tam() {
        se a[i] != b[i] {
            ret 0
        }
    }
    ret 1
}

erros = 0
para t=0 ate 40 {
    n = proximo() % 700
    u8 = {}
    i16 = {}
    grande = {}
    para i=0 ate n {
        u8.adicionar(proximo() % 200)
        i16.adicionar(proximo() % 60000 - 30000)
        grande.adicionar(proximo() * 4398046511 - 4611686018427387904)
    }
    a = copia(u8)
    b = copia(u8)
    ordenar(a)
    insercao(b)
    erros = erros + 1 - iguais(a, b)
    a = copia(i16)
    b = copia(i16)
    ordenar(a)
    insercao(b)
    erros = erros + 1 - iguais(a, b)
    a = copia(grande)
    b = copia(grande)
    ordenar(a)
    insercao(b)
    erros = erros + 1 - iguais(a, b)
    ordenar(grande, maior)
    para i=1 ate n {
        se grande[i - 1] < grande[i] {
            erros = erros + 1
        }
    }
}
escrever("erros:", erros)

// tuplas com muitos repetidos (testa a partição de Hoare)
tp = {}
para i=0 ate 5000 {
    tp.adicionar((proximo() % 3, proximo() % 3))
}
ordenar(tp)
ok = 1
para i=1 ate 5000 {
    se tp[i] < tp[i - 1] {
        ok = 0
    }
}
escrever("tuplas ordenadas:", ok)

// vetores estreitos de verdade (u8, i16): cada um só com seus valores
b8 = {}
b16 = {}
para i=0 ate 3000 {
    b8.adicionar(proximo() % 250)
    b16.adicionar(proximo() % 60000 - 30000)
}
s8 = soma(b8)
s16 = soma(b16)
ordenar(b8)
ordenar(b16, maior)
ok = 1
para i=1 ate 3000 {
    se b8[i] < b8[i - 1] ou b16[i] > b16[i - 1] {
        ok = 0
    }
}
escrever("estreitos ordenados:", ok, s8 == soma(b8), s16 == soma(b16))

// os demais tamanhos: i8, u16, u32, i32
c8 = {}
c16 = {}
c32 = {}
d32 = {}
para i=0 ate 2000 {
    c8.adicionar(proximo() % 200 - 100)
    c16.adicionar(proximo() % 60000)
    c32.adicionar(proximo() * 2 % 4000000000)
    d32.adicionar(proximo() % 2000000000 - 1000000000)
}
ordenar(c8)
ordenar(c16)
ordenar(c32)
ordenar(d32)
ok = 1
para i=1 ate 2000 {
    se c8[i] < c8[i - 1] ou c16[i] < c16[i - 1] ou c32[i] < c32[i - 1] ou d32[i] < d32[i - 1] {
        ok = 0
    }
}
escrever("i8 u16 u32 i32 ordenados:", ok)
