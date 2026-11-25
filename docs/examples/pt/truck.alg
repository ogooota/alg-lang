#lang pt
fn cabe(pesos, cap, k) {
    viagens = 1
    carga = 0
    para p em pesos {
        se p > cap {
            ret 0
        }
        se carga + p > cap {
            viagens++
            carga = 0
        }
        carga += p
    }
    ret viagens <= k
}

ler(n, k, pesos[0..n])
lo = 1
hi = soma(pesos)
enquanto lo < hi {
    meio = (lo + hi) / 2
    se cabe(pesos, meio, k) {
        hi = meio
    } senao {
        lo = meio + 1
    }
}
escrever(lo)
