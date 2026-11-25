# Como o compilador funciona

`fonte .alg → tokenize → parse → check_program → analyze_ranges → generate_c → cc`

## Módulos

| Arquivo | Responsabilidade |
|---|---|
| `alg.h` | tipos compartilhados (tokens, tipos, AST, programa) e a interface de cada fase |
| `util.c` | alocação, buffer de texto, mensagens de erro |
| `language.c` | palavras-chave, nomes nativos e métodos em cada idioma (o único lugar com palavras pt/en) |
| `lex.c` | texto → tokens |
| `parse.c` | tokens → AST |
| `sema_internal.h` | estado e funções compartilhados pela checagem semântica |
| `sema_types.c` | tipos: unificação, células de elemento, assinaturas |
| `sema_scope.c` | escopos (tabela hash por escopo), variáveis, cópia de corpos, versões de função |
| `sema_expr.c` | checagem de expressões, chamadas nativas e métodos |
| `sema.c` | checagem de comandos e a entrada `check_program` |
| `range.c` | análise de faixa (tamanho dos elementos dos vetores) |
| `gen_internal.h` | estado e funções compartilhados pela geração de C |
| `gen_types.c` | tipo C de cada tipo da linguagem, tipos de elemento, instâncias de template |
| `gen_lazy.c` | análise do `#decl` (checagem fora dos laços) e variáveis usadas em funções |
| `gen_expr.c` | expressões, chamadas nativas, ler/escrever |
| `gen_stmt.c` | comandos e laços |
| `gen.c` | emissão final: runtime, templates usados, funções, `alg_main` |
| `main.c` | linha de comando, chama o `cc` |

A divisão não muda o C gerado: o teste de regressão da modularização comparou byte a byte a saída de todos os programas de `tests/`, `bench/` e `docs/examples/`, com e sem `-O`.

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
| `STMT_WHILE` | `enquanto first: body`; `else_body` é o passo do `para (início; cond; passo)` do C, gerado como `for (; cond; ({ passo }))` para rodar também no `continue`. O parser transforma `para (i = a; i < b; i++)` em `STMT_FOR` quando nem `i` nem `b` mudam no corpo (sem atribuição, leitura, método ou chamada de função do usuário) |
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

## Strings

- Uma string é um `ArrayU8` (vetor 1D de `TYPE_CHAR`); `is_string`/`is_text_type` reconhecem esse tipo.
- Dentro de vetor, heap, fila ou mapa a célula tem tipo `TYPE_STRING` e é guardada como `Str`, de 16 bytes, no formato das strings do Umbra (Neumann e Freitag, CIDR 2020).
  - Formato: tamanho, os 4 primeiros bytes e um ponteiro (`STRING_ELEMENT`, `KIND 3`).
  - Guardar: `str_pack` copia o texto.
  - Ler: `str_unpack` devolve uma view com `capacity = length`, então o primeiro `add` realoca.
  - Comparar: tamanho e prefixo num único load de 64 bits decidem a maioria dos casos sem seguir o ponteiro.
  - Ganho com 10⁶ palavras: de 0,54 s e 135 MB para 0,28 s e 68 MB.
- `rt/string.h` entra logo depois do template `ArrayU8`. Ele tem leitura de palavra e de linha, escrita, comparação, hash FNV-1a e junção.
- `str_sort` usa `order_sort` (`core.h`) e faz a permutação no lugar pelos ciclos.
  - `order_sort` faz radix LSD sobre um prefixo de 64 bits que preserva a ordem, pulando os bytes iguais em todas as chaves.
  - O desempate só acontece nos blocos de prefixo igual: inserção até 16 itens, `qsort` acima disso, nunca O(n²).
  - Medido: 0,81× o tempo do `std::sort` de `vector<string>`, com 10⁶ palavras.
- `read_line` logo depois de um número pula o resto vazio da linha (`input_after_token`), como o `getline` depois de `cin >> ws` faria.


## Alinhamento, peeling e unroll (medido num i5-7200U, AVX2)

- **Alinhamento:** `arena_allocate` começa todo bloco de 1 KB ou mais numa linha de cache (64 bytes).
  - Com base de 16 bytes, metade dos loads e stores AVX de 32 bytes atravessa duas linhas.
  - Laço vetorizado que lê e grava, com dados no L1/L2: **0,78×** do tempo.
  - Laço que só lê (redução): sem diferença.
  - Custo: até 63 bytes por bloco grande. Blocos pequenos (strings curtas, nós) não mudam.
- **Peeling:** o gcc 13 com `-mtune=generic` não faz peeling de alinhamento em nenhum laço dos benchmarks. Ele usa loads desalinhados (`vmovdqu`), que custam o mesmo que os alinhados quando o endereço está alinhado. Peeling só alinharia *um* dos vetores do laço; alinhar na alocação acerta todos de uma vez, sem custo em tempo de execução. Por isso não geramos peeling nem `__builtin_assume_aligned`: o segundo seria falso em views (`a[i..j]`, linhas de matriz), que começam em qualquer endereço.
- **Unroll:** o `unroll-loops` do `-O` fica.
  - Ganho de 10% nos laços vetorizados (redução, mochila).
  - Nos outros benchmarks, a diferença ficou dentro de ±3%, que é ruído.

## Mapa (rt/map.h)

- **Layout:**
  - Uma struct `{chave, valor}` por posição: a busca que acha a chave já traz o valor para o cache.
  - Um byte de controle por posição: 0 vazia, 1 removida, `0x80 | 7 bits do hash` quando ocupada. A ideia vem da SwissTable do Abseil; a sondagem só compara a chave quando os 7 bits batem.
- **`m[k] op= x`** gera uma busca só (`gen_stmt.c`, `cached_map_index`). Antes eram duas: `get` e `ref`.
- **`for [k, v] in m`** usa `_entries`:
  - Copia os pares numa passada sequencial e ordena com `order_sort` pelo `ORDER_PREFIX`: inteiro com o sinal invertido, ou os 8 bytes do texto. Chaves tupla e real não têm prefixo e caem no `qsort`.
  - O valor sai da foto enquanto `map->writes` não muda, ou seja, enquanto o corpo não chama `_ref` nem `_del`. Se mudar, o valor é buscado de novo, como antes.

## Crivo (`sieve`, rt/core.h)

O desenho segue o primesieve (Kim Walisch, `doc/ALGORITHMS.md`):
- **Roda módulo 30:** o byte b guarda 30b + {1, 7, 11, 13, 17, 19, 23, 29}, um bit cada. Múltiplos de 2, 3 e 5 não existem no vetor.
- **Segmentos de 32 KB**, o tamanho do L1.
- **Pré-crivo:** um padrão de 17.017 bytes com os múltiplos de 7, 11, 13 e 17 já riscados é copiado para cada segmento.
- **Riscar:** o múltiplo p·q com q = 30t + r_k fica no byte p·t + ⌊p·r_k/30⌋. Cada primo guarda os 8 deslocamentos e as 8 máscaras de uma volta, e o laço desenrolado risca 8 de uma vez e avança p bytes.
- **Extrair:** 8 bytes por vez com `ctz`. O vetor de saída é reservado por π(x) < 1,25506·x/ln x (Rosser e Schoenfeld), sem crescer.
- **Medido:**

| limite | crivo ingênuo | só ímpares, 1 bit | roda 30 |
|---|---|---|---|
| 2·10⁷ | 0,203 s | 0,031 s | **0,021 s** |
| 10⁸ | 1,065 s | 0,134 s | **0,092 s** |
| 10⁹ | | 1,429 s | **0,929 s** |

Em 10⁹, a maior parte do tempo é gravar 50,8 milhões de inteiros (406 MB).
- **Não feito:**
  - Buckets de Oliveira e Silva para primos grandes: só compensam com limites acima de ~10¹⁰, que não cabem num vetor de saída.
  - Pré-crivo com SIMD até 163.

