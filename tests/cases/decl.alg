#lang pt
#decl {
    dp[n + 1][m + 1];
    cont; media: real;
}

ler(n, m)
ler(grid[0..n][0..m])
para i=0 ate n {
    para j=0 ate m {
        acima = 0
        esquerda = 0
        se i > 0 {
            acima = dp[i - 1][j]
        }
        se j > 0 {
            esquerda = dp[i][j - 1]
        }
        dp[i][j] = grid[i][j] + max(acima, esquerda)
    }
}
escrever(dp[n - 1][m - 1])
escrever(grid)

fn conta(lim) {
    para i=0 ate lim {
        se i % 3 == 0 {
            continue
        }
        se i > 10 {
            pare
        }
        cont = cont + 1
    }
}

conta(100)
escrever("cont", cont)
media = soma(grid[0]) / 2.0
escrever(media)

fn desce(k) {
    se k == 0 {
        ret 0
    }
    ret desce(k - 1) + 1
}
escrever(desce(1000000))

fn zera(w) {
    para i=0 ate tam(w) {
        w[i] = 0
    }
}
linha = copia(grid[1])
zera(grid[0])
escrever(grid[0], linha)
x = 5
se x < 3 {
    escrever("menor")
} senao se x < 10 {
    escrever("medio")
} senao {
    escrever("grande")
}
