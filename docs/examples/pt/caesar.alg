#lang pt
#decl linha: texto
ler(k)
ler_linha(linha)
resposta = ""
para c em linha {
    se (c >= 'a' && c <= 'z')
        c = 'a' + (c - 'a' + k) % 26
    resposta.adicionar(c)
}
escrever(resposta)
