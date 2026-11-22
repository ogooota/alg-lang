# Como o compilador funciona

`fonte .alg → tokenize (lex.c) → parse (parse.c) → check_program (sema.c) → analyze_ranges (range.c) → generate_c (gen.c) → cc`

## AST: uso dos campos `first`, `second`, `third`

| Expressão | Significado |
|---|---|
| `EXPR_BINARY` | `first op second` |
| `EXPR_UNARY` | `op first` |
| `EXPR_INDEX` | `first[second]` (vetor ou mapa) |
| `EXPR_SLICE` | `first[second..third]` |
| `EXPR_METHOD` | `first.text(arguments)` |
| `EXPR_GENERATOR` | `first para text=second ate third`, ou `first para text em third` (`second == NULL`) |

| Comando | Significado |
|---|---|
| `STMT_ASSIGN` | `first = second` (`first` pode ser uma tupla) |
| `STMT_IF` | `se first: body senao: else_body` |
| `STMT_WHILE` | `enquanto first: body` |
| `STMT_FOR` | `para text=first ate second: body` |
| `STMT_FOR_IN` | `para text em first: body`, ou `para (parameters) em first: body` |
| `STMT_RETURN` | `ret first` |
| `STMT_FUNCTION` | `fn text(parameters): body` |

## Análise de faixa (`range.c`)

Descobre o menor e o maior valor que cada vetor de inteiros pode guardar; o gerador usa isso para escolher 1, 2, 4 ou 8 bytes por elemento. Invariantes:

- toda variável e todo vetor começa com a faixa [0, 0] (o C zera tudo);
- cada escrita só alarga a faixa (junção), então o processo é monótono;
- uma faixa que cresce mais de 4 vezes vira "tudo" (alargamento), o que garante que a repetição termina;
- valor de origem desconhecida (entrada, parâmetro, retorno de função, conta que estoura, valor tirado de heap, fila ou mapa) é sempre "tudo": nenhum tamanho menor é escolhido sem prova.

## Runtime (`rt/`)

Cada arquivo é um template: o gerador define macros e inclui o texto uma vez por tipo usado.

**Vetor (`arr.h`)**: `ELEMENT` (tipo guardado: `uint8_t` ... `int64_t`, `double`, tupla), `VALUE` (tipo das contas e do que sai do vetor), `ARRAY` (nome da struct, `ArrayU8`), `FN(name)` (prefixo das funções, `array_u8##name`), `READ_ELEMENT`, `WRITE_ELEMENT`, `LESS(left, right)`, `LOAD(stored)` (guardado → valor; o int32 com INF decodifica) e `KIND` (0 inteiro, 1 real, 2 tupla). Um vetor de k dimensões é um bloco contíguo; linhas e fatias são views do mesmo bloco, com `capacity = -1` (não crescem).

**Ordenação**: radix sort LSD para inteiros. A chave tem a largura do elemento; em tipo com sinal, o bit de sinal dessa largura é invertido, e a ordem sem sinal dos bytes vira a ordem dos inteiros. Uma passada lê todos os histogramas; um byte igual em todos os elementos é pulado (vetor estreito vira counting sort). Reais e tuplas usam introsort: pivô mediana de três com um sorteado, partição de Hoare, heapsort se a recursão afunda e inserção nos pedaços pequenos.

**Heap (`heap.h`)**: `ELEMENT`, `HEAP`, `FN`, `LESS` (já invertido no `heap(maior)`). Invariante: `data[0..count)` é um heap; subir e descer usam "buraco" (move em vez de trocar), como o `std::priority_queue`.

**Fila (`queue.h`)**: buffer circular; `capacity` é potência de 2, então o módulo é um `&`.

**Mapa (`map.h`)**: `KEY`, `MAP_VALUE`, `MAP`, `FN`, `HASH(key)` (já misturado com a semente aleatória da execução), `KEYS_EQUAL`, `KEY_ARRAY`/`KEY_ARRAY_FN` (para percorrer em ordem). Invariantes: `capacity` é potência de 2; `states[i]` é 0 (vazio), 1 (ocupado) ou 2 (removido); `used_slots` (ocupados + removidos) fica abaixo de `capacity / 2`, então toda sondagem acha um vazio. Percorrer usa uma cópia ordenada das chaves.

**Grafo (`graph.h`)**: `WEIGHT`, `GRAPH`, `FN`. As arestas entram numa lista (`aresta` é O(1)); na primeira consulta a `vizinhos`, a lista vira adjacência compacta (CSR) em O(n + m): os vizinhos de u são `adjacency[adjacency_start[u] .. adjacency_start[u + 1])`, na ordem em que foram adicionados. Uma aresta nova marca o grafo para remontar na próxima consulta. A adjacência fica na arena, então um laço que adiciona arestas no meio continua percorrendo a versão antiga, sem ponteiro inválido.
