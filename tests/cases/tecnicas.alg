#lang pt
// cada técnica nativa comparada com a força bruta, em valores aleatórios
semente = 7

fn proximo() {
    semente = (semente * 1103515245 + 12345) % 2147483648
    ret semente
}

erros = 0
v = {}
para i=0 ate 300 {
    v.adicionar(proximo() % 1000 - 500)
}

// prefixo 1D
p = prefixo(v)
para k=0 ate 200 {
    l = proximo() % 301
    r = l + proximo() % (301 - l)
    bruta = 0
    para i=l ate r {
        bruta += v[i]
    }
    se p[r] - p[l] != bruta {
        inc erros
    }
}

// prefixo 2D
ler(nl, nc, mat[0..nl][0..nc])
pm = prefixo(mat)
para a=0 ate nl {
    para b=a + 1 ate em nl {
        para c=0 ate nc {
            para fim=c + 1 ate em nc {
                bruta = 0
                para i=a ate b {
                    para j=c ate fim {
                        bruta += mat[i][j]
                    }
                }
                se pm[b][fim] - pm[a][fim] - pm[b][c] + pm[a][c] != bruta {
                    inc erros
                }
            }
        }
    }
}
escrever("prefixo ok:", erros == 0)

// busca (vetor ordenado)
o = copia(v)
ordenar(o)
para k=0 ate 300 {
    x = proximo() % 1200 - 600
    primeiro = 0
    enquanto primeiro < tam(o) e o[primeiro] < x {
        inc primeiro
    }
    depois = 0
    enquanto depois < tam(o) e o[depois] <= x {
        inc depois
    }
    se busca(o, x) != primeiro ou busca(o, x, maior) != depois {
        inc erros
    }
}
escrever("busca ok:", erros == 0)

// matemática
escrever(mdc(12, 18), mdc(-12, 18), mdc(0, 5), mmc(4, 6), mmc(0, 3))
escrever(potmod(2, 10, 1000), potmod(-2, 3, 7), potmod(3, 0, 5), potmod(123456789, 1000000006, 1000000007))
primos = crivo(30)
escrever(primos, tam(crivo(100000)))

// fenwick e segtree contra vetor comum, com mudanças
f = fenwick(300)
s_min = segtree(v, min)
s_max = segtree(v, max)
s_soma = segtree(v, soma)
r_min = rmq(v, min)
para i=0 ate 300 {
    f.somar(i, v[i])
}
w = copia(v)
para k=0 ate 300 {
    l = proximo() % 300
    r = l + 1 + proximo() % (300 - l)
    menor = w[l]
    maior_ = w[l]
    total = 0
    para i=l ate r {
        menor = min(menor, w[i])
        maior_ = max(maior_, w[i])
        total += w[i]
    }
    se f.soma(l, r) != total ou s_min.consulta(l, r) != menor ou s_max.consulta(l, r) != maior_ ou s_soma.consulta(l, r) != total {
        inc erros
    }
    // muda uma posição em todas
    i = proximo() % 300
    novo = proximo() % 1000 - 500
    f.somar(i, novo - w[i])
    s_min.mudar(i, novo)
    s_max.mudar(i, novo)
    s_soma.mudar(i, novo)
    w[i] = novo
}
escrever("fenwick/segtree ok:", erros == 0)

// rmq (vetor original, sem mudanças)
para k=0 ate 300 {
    l = proximo() % 300
    r = l + 1 + proximo() % (300 - l)
    se r_min.consulta(l, r) != min(v[l..r]) {
        inc erros
    }
}
escrever("rmq ok:", erros == 0, tam(r_min))

// union-find: Kruskal
arestas = {(4, 0, 1), (1, 1, 2), (3, 0, 2), (2, 2, 3), (5, 3, 4), (7, 1, 4)}
ordenar(arestas)
d = conjuntos(5)
custo = 0
para (peso, a, b) em arestas {
    se d.unir(a, b) {
        custo += peso
    }
}
escrever(custo, d.grupos(), d.juntos(0, 4), d.tam(2), d.achar(3) == d.achar(0))
