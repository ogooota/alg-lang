# alg

Linguagem para estudar algoritmos e competir em maratonas: lê como pseudocódigo,
compila para C e roda tão rápido quanto C++ (ou mais).

```
#lang pt
ler(n, v[0..n])
ordenar(v)
escrever(soma(v), max(v))
```

## Uso

```sh
make                       # gera ./alg
./alg prog.alg < entrada   # compila e roda
./alg -O -o prog prog.alg  # executável sem checagens
make test                  # testes
python3 bench/gen_inputs.py && python3 bench/run.py   # benchmark contra C++
```

- Guia com exemplos de maratona: `docs/guia.html`
- Especificação completa: `docs/linguagem.md`
- Destaque de sintaxe para nvim: `editors/nvim/`
