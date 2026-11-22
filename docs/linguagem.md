# alg: especificação

Linguagem para estudar algoritmos com cara de pseudocódigo e performance de C.
`alg` é um compilador: análise léxica, sintática e de tipos, depois geração de C e `cc`.

Legenda: ✅ implementado · ⏳ decidido, ainda não implementado

## Uso

```sh
make                          # gera ./alg
./alg prog.alg < entrada.txt  # compila e roda
./alg -o prog prog.alg        # gera o executável
./alg --emit-c prog.alg       # mostra o C gerado
./alg -O prog.alg             # sem checagens de índice/divisão (competitiva)
make test                     # roda tests/cases/*.alg
```

## Decisões

| Tema | Decisão | Status |
|---|---|---|
| Idioma | `#lang pt` ou `#lang en` obrigatório na 1ª linha; um idioma por arquivo | ✅ |
| Blocos | indentação (tab = 4 espaços); `se x: y = 1` em uma linha também vale | ✅ |
| Comentário | `//` | ✅ |
| `;` | opcional no fim de qualquer comando (`x = 3;`), para quem está acostumado com C | ✅ |
| Função | `fn nome(a, b):` com bloco, ou `f(x) = expr` em uma linha; `ret` devolve (iguais em pt e en) | ✅ |
| Tipos | estáticos e inferidos; `int` (64 bits), `real`, `int32` — iguais em pt e en | ✅ |
| `: int32` | `#decl dp[n][m]: int32` guarda em 32 bits; valor que não cabe é erro (sem `-O`) | ✅ |
| Vetor estreito | o compilador prova a faixa de valores e usa 1, 2, 4 ou 8 bytes por elemento | ✅ |
| `-O` | sem checagens + `#pragma GCC optimize("O3,unroll-loops")` e `target("avx2")` no C gerado | ✅ |
| Atribuição | `=` atribui (e declara, se for nova); `==` compara | ✅ |
| `#decl` | variáveis não inicializadas (começam zeradas), no topo do arquivo ou da função | ✅ |
| `#decl v[n]` | o vetor é criado e zerado no primeiro uso (`n` pode ser lido depois) | ✅ |
| Índices | começam em 0; checados (erro com linha) a não ser com `-O` | ✅ |
| Intervalo | `a[i..j]` não inclui `j`; `para i=0 ate n` = 0..n-1; `ate em n` = 0..n | ✅ |
| Fatia | enxerga os mesmos dados (view); `copia(...)` faz cópia | ✅ |
| Divisão | `/` entre inteiros arredonda para baixo; `%` nunca é negativo | ✅ |
| Lógica | `e` / `ou` / `nao`; bit a bit `&` `\|` `^` `~` `<<` `>>` | ✅ |
| Atualização | `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` (iguais ao C); `inc x` / `dec x` no lugar de `++` / `--` (iguais em pt e en); só como comando, nunca dentro de expressão | ✅ |
| Precedência | bit a bit liga mais forte que comparação (`x & 1 == 0` funciona) | ✅ |
| Funções | `fn f(x):` + `ret`, ou `f(x) = expr`; tipos inferidos pelas chamadas | ✅ |
| Escopo | atribuir a uma global dentro de função altera a global; a variável do `para` dentro de função é sempre local | ✅ |
| Passagem | número por valor, vetor por referência | ✅ |
| Programa | sem `main`: o topo do arquivo é o programa; `ret` encerra | ✅ |
| Entrada | `ler(a, b, v[0..n])`: da esquerda para a direita, cria o vetor se preciso | ✅ |
| Saída | `escrever(a, b)` separa por espaço e sempre termina com `\n` | ✅ |
| Memória | arena por execução (tudo é liberado no fim) | ✅ |
| Pilha | o programa roda com 256 MB de pilha (recursão funda) | ✅ |
| Unicode | `≤ ≥ ≠ ← ∈` como sinônimos opcionais | ✅ |
| Texto | `ler(s)`, `s[i]`, `s[i..j]`, `tam(s)`, `==`, `<` | ⏳ |
| Conjunto | `conj {}`; o compilador escolhe hash ou árvore pelo uso | ⏳ |
| Compreensão | `{x em v se x > 0}` vetor; `conj {...}` conjunto | ⏳ |
| Operações de conjunto | `uniao`, `inter`, `menos` (o `em` já funciona com mapa) | ⏳ |
| Quantificadores | `para todo`, `existe` / `para algum` | ⏳ |
| `INF` | constante 10¹⁸ (folga para `INF + peso`); em vetor `: int32` vira um valor especial e volta a ser 10¹⁸ na leitura | ✅ |
| Tuplas | `(d, v)`, comparação lexicográfica, `(a, b) = ...`, `para (a, b) em v` | ✅ |
| Ordenar | `ordenar(v)`, `ordenar(v[i..j])`, `ordenar(v, maior)`; tuplas em ordem lexicográfica | ✅ |
| Mapa | `m = mapa {}`, `m[k]` (ausente vale 0), `k em m`, `m.remover(k)`, `para (k, x) em m` (chaves em ordem) | ✅ |
| Heap | `heap()` (menor sai primeiro), `heap(maior)`; `inserir`, `remover`, `topo` | ✅ |
| Fila | `fila()`; `enfileirar`, `desenfileirar`, `frente` | ✅ |
| Pilha | o próprio vetor: `adicionar`, `remover_ultimo` | ✅ |
| Grafo | `grafo(n)`, `grafo(n, dirigido)`; `g.aresta(u, v[, peso])`; `para v em g.vizinhos(u)`, `para (v, w) em g.vizinhos(u)` | ✅ |
| Registros | `tipo Aresta: u; v; peso` | ⏳ |
| Adiado | `rep(ação, parada)`, passo/decrescente no `para`, tradutor pt↔en | — |

## O que já dá para escrever (marco 1)

```
#lang pt
#decl
    dist[n];            // criado no 1º uso, quando n já foi lido
    total; media: real; // zerados

ler(n, m)               // vários de uma vez
ler(grid[0..n][0..m])   // cria a matriz n×m e lê
ler(v)                  // v já existe: lê todos os elementos

x = 5                   // declara
y = 2.5                 // real
v = {}                  // vetor vazio
w = {1, 2, 3}
v.adicionar(4)
u = v.remover_ultimo()
tam(v)
copia(v[1..3])

se x > 0 e nao (x == 3):
    escrever("ok")
senao se x < -10:
    escrever("muito negativo")
senao:
    escrever("outro")

enquanto lo < hi:
    m = (lo + hi) / 2
    ...
    pare / continue

para i=0 ate n:         // 0..n-1
para i=1 ate em n:      // 1..n
para x em v:            // elementos
para linha em grid:     // linhas (vetor 1D)

soma(v)  max(v)  min(v)  min(a, b, c)
soma(i * i para i=0 ate n)
max(x para x em v)
int(2.7)  real(3)

quadrado(x) = x * x     // função de uma linha
fn fib(k):
    se k < 2:
        ret k
    ret fib(k - 1) + fib(k - 2)

escrever(a, "texto", v, grid)
```

## Palavras e nomes nativos: pt ↔ en

| pt | en | | pt | en |
|---|---|---|---|---|
| `se` | `if` | | `ler` | `read` |
| `senao` | `else` | | `escrever` | `write` |
| `enquanto` | `while` | | `tam` | `len` |
| `para` | `for` | | `copia` | `copy` |
| `ate` | `to` | | `soma` | `sum` |
| `ate em` | `thr` | | `min` / `max` | `min` / `max` |
| `em` | `in` | | `adicionar` | `add` |
| `fn` | `fn` | | `remover_ultimo` | `pop` |
| `ret` | `ret` | | `int` / `int32` | `int` / `int32` |
| `pare` | `break` | | `real` | `real` |
| `continue` | `continue` | | `todo` ⏳ | `all` |
| `ordenar` / `maior` | `sort` / `greater` | | `heap` / `inserir` / `remover` / `topo` | `heap` / `push` / `pop` / `top` |
| `fila` / `enfileirar` / `desenfileirar` / `frente` | `queue` / `push` / `pop` / `front` | | `grafo` / `dirigido` / `aresta` / `vizinhos` | `graph` / `directed` / `edge` / `neighbors` |
| `mapa` / `remover` | `map` / `remove` | | `INF` | `INF` |
| `e` / `ou` / `nao` | `and` / `or` / `not` | | `existe` ⏳ | `exists` |
| `verdadeiro` / `falso` | `true` / `false` | | `algum` ⏳ | `some` |
| `uniao` ⏳ | `union` | | `conj` ⏳ | `set` |
| `inter` ⏳ | `inter` | | `tipo` ⏳ | `type` |
| `menos` ⏳ | `minus` | | | |

As palavras marcadas com ⏳ já são reservadas, para que nenhum programa precise mudar quando elas entrarem.

## Estrutura do compilador

| Arquivo | Papel |
|---|---|
| `src/lex.c` | tokens, indentação → INDENT/DEDENT, palavras por idioma |
| `src/parse.c` | AST (descida recursiva) |
| `src/sema.c` | escopos, inferência de tipos, uma versão de função por combinação de tipos |
| `src/gen.c` | AST tipada → C |
| `rt/core.h` | runtime: arena, entrada/saída rápida, erros, divisão euclidiana |
| `rt/arr.h` | template de vetor (incluído uma vez por tipo de elemento) |

Por enquanto a AST tipada faz o papel de representação intermediária; um IR próprio (três endereços) entra quando chegarem as primeiras otimizações (escolha hash × árvore, laços).

## Desempenho

`python3 bench/run.py` compara com C++ idiomático de competição (mesma máquina, melhor de 5).

| Problema | alg -O | C++ -O2 | C++ com os mesmos pragmas |
|---|---|---|---|
| prefix (I/O 10⁶) | 0,214 s | 0,381 s | 0,390 s |
| knap (10⁸ ops) | 0,045 s | 0,080 s | 0,045 s |
| sieve (2·10⁷) | 0,181 s | 0,186 s | 0,189 s |
| fib(38) | 0,097 s | 0,095 s | 0,089 s |
| lcs 4000² | 0,074 s | 0,078 s | 0,078 s |

O que faz o alg ser rápido:
- leitura/escrita próprias (2× mais rápidas que `cin`/`cout` sem sync);
- análise de faixa (`src/range.c`): vetores com o menor tipo que guarda os valores provados;
- checagem do `#decl` feita uma vez antes do laço (quando o tamanho não muda nele);
- variáveis do programa que nenhuma função usa viram locais (o gcc otimiza melhor);
- pragmas no próprio C gerado (valem no juiz, que compila o arquivo como ele é).

### Estruturas (alg -O × C++ -O2, mesma entrada)

| Problema | alg -O | C++ com `cin` | C++ com leitor rápido (`bench/fastin.h`) |
|---|---|---|---|
| ordenar 10⁶ inteiros | 0,085 s | 0,187 s | 0,115 s |
| Dijkstra, 10⁵ vértices, 10⁶ arestas | 0,168 s | 0,468 s | 0,269 s |
| BFS, 10⁶ vértices, 2·10⁶ arestas | 0,627 s | 1,402 s | 1,205 s |
| mapa: contar 10⁶ valores | 0,156 s | 0,297 s | 0,182 s |

Como cada estrutura é feita (`rt/`):
- **ordenar**: radix sort LSD para inteiros (pula bytes iguais em todos, então vetor estreito vira counting sort); introsort (mediana de três com pivô sorteado + heapsort + inserção) para reais e tuplas;
- **heap**: binário com "buraco" (move em vez de trocar);
- **fila**: buffer circular de potência de 2;
- **grafo**: arestas numa lista; na primeira consulta viram adjacência compacta (CSR) em O(n + m). Adicionar arestas depois disso faz remontar na próxima consulta: monte o grafo antes de percorrer;
- **mapa**: hash com endereçamento aberto e semente aleatória por execução (entrada feita para derrubar o hash não funciona). Percorrer um mapa ordena as chaves (O(k log k) por percurso) e trabalha numa cópia, então mudar o mapa dentro do laço é seguro.

## Convenções do código C (compilador e runtime)

Todo C do projeto segue estas regras (o `.clang-format` cuida da formatação; `make format`):

- **Chaves sempre na própria linha** (estilo Allman), em todo escopo: `int main()` / `{`.
- **Uma declaração por linha**, inclusive dentro de struct (nunca `int a, b, c;`).
- **Tipos em PascalCase**: structs, enums e typedefs (`Expression`, `TokenKind`, `ArrayU8`, `HeapI64Min`).
- **Variáveis, campos e funções em snake_case com nome completo** (`expr`, `stmt`, `cell_element_kind`, `vertex_count`). Exceção aceita: `i`, `j` como contadores de laço.
- **Constantes de enum e macros em MAIÚSCULAS com prefixo do grupo** (`TOKEN_NEWLINE`, `EXPR_INDEX`, `TYPE_ARRAY`, `KEYWORD_SE`).
- **Linhas em branco** entre blocos lógicos, depois de `}` e antes de `if`/`for`/`while`; todo comentário em linha própria tem uma linha em branco antes.
- **Documentação** (`/** ... */`) acima de cada função, dizendo o que ela faz e o que devolve.
- No C gerado: tipos do runtime em PascalCase (`ArrayI64`), funções em snake_case (`array_i64_push`, `read_int`, `write_space`).

## TODO

- **Idiomas configuráveis**: hoje pt e en estão em tabelas dentro do código (`src/lex.c` e `src/parse.c`). A ideia é tirar as palavras-chave, os nomes nativos e os métodos para arquivos de idioma (um por idioma), para que qualquer pessoa traduza a linguagem só criando uma configuração nova (`#lang es`, `#lang fr`...). Importante quando o projeto virar open source.
