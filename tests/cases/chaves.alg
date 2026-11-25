#lang pt
// a chave pode vir na mesma linha ou na seguinte; a indentação é livre
fn maior_de(a, b)
{
    se a > b
    {
        ret a
    }
    senao
    {
        ret b
    }
}

v = {
    3, 1,
    2
}
se v.tam() == 3 { escrever("três") }
para x em v {
escrever(x, maior_de(x, 2))
}
i = 0
enquanto i < 2 { i++; }
se i == 2 {
    escrever("i", i)
} senao se i == 3 {
    escrever("nunca")
} senao {
    escrever("nunca")
}
