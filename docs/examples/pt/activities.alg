#lang pt
ler(n)
tarefas = {}
para i=0 ate n {
    ler(inicio, fim)
    tarefas.adicionar((fim, inicio))
}
ordenar(tarefas)
total = 0
livre = 0
para [fim, inicio] em tarefas {
    se inicio >= livre {
        total++
        livre = fim
    }
}
escrever(total)
