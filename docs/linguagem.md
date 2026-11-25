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
| Blocos | chaves `{ }`, com a chave aberta na mesma linha (`se x {`) ou na seguinte; indentação livre; `se x { y = 1 }` em uma linha também vale. Como em C, corpo de um comando só dispensa as chaves (`para i=0 ate n` ⏎ `se i > 5` ⏎ `escrever(i)`), e o `senao` fica com o `se` mais próximo | ✅ |
| Comentário | `//` | ✅ |
| `;` | opcional no fim de qualquer comando (`x = 3;`), para quem está acostumado com C | ✅ |
| Parênteses do C | opcionais em `se (x > 0)`, `enquanto (i < n)` e `para (...)`: as duas formas valem, com ou sem. Com parênteses, o corpo de um comando só cabe na mesma linha: `se (x) escrever(x) senao escrever(0)` | ✅ |
| `para` | `para i=0 ate n`, `para i=0 ate em n` (en: `thr`), `para x em v`, `para [a, b] em v`; com parênteses: `para (i=0 ate n)`, `para (x em v)`, `para ([a, b] em v)`; e o do C: `para (i = 0; i < n; i++)`, com qualquer condição e passo (`i--`, `i += 2`, `i *= 2`; partes vazias em `para (;;)`) | ✅ |
| Função | `fn nome(a, b) { ... }`, ou `f(x) = expr` em uma linha; `ret` devolve (iguais em pt e en). Tipos opcionais no estilo do Rust: `fn f(v: {int}, k: int) -> int` (cada parte pode faltar) | ✅ |
| Tipos | estáticos e inferidos; `int` (64 bits), `real`, `int32`, `str` — iguais em pt e en. Escrever o tipo é opcional e se escreve como o literal: `{int}` vetor, `{{int}}` matriz, `(int, str)` tupla, `map {str: int}` mapa (en: `map`, pt: `mapa`), `heap {int}` / `heap(greater) {int}`, `queue {int}` e `graph` / `graph(directed)` (este só em parâmetro, porque o grafo precisa de `n`). Vale no `#decl`, nos parâmetros e no retorno; valor que não combina é erro de compilação | ✅ |
| `: int32` | `#decl dp[n][m]: int32` guarda em 32 bits; valor que não cabe é erro (sem `-O`) | ✅ |
| Vetor estreito | o compilador prova a faixa de valores e usa 1, 2, 4 ou 8 bytes por elemento | ✅ |
| `-O` | sem checagens + `#pragma GCC optimize("O3,unroll-loops")` e `target("avx2")` no C gerado | ✅ |
| Atribuição | `=` atribui (e declara, se for nova); `==` compara | ✅ |
| `#decl` | variáveis não inicializadas (começam zeradas), no topo do arquivo ou da função; com tipo: `#decl item: {(int, int)}` (e `ler(n, item[0..n])` lê `n` pares), `#decl m: map {str: int}` (já nasce vazio), `#decl melhor: (int, str)` (`ler(melhor)` lê os dois campos) | ✅ |
| `#decl v[n]` | o vetor é criado e zerado no primeiro uso (`n` pode ser lido depois) | ✅ |
| Índices | começam em 0; checados (erro com linha) a não ser com `-O` | ✅ |
| Intervalo | `a[i..j]` não inclui `j`; `para i=0 ate n` = 0..n-1; `ate em n` = 0..n | ✅ |
| Fatia | enxerga os mesmos dados (view); `copia(...)` faz cópia | ✅ |
| Divisão | `/` entre inteiros arredonda para baixo; `%` nunca é negativo | ✅ |
| Lógica | `e` / `ou` / `nao` (en: `and` / `or` / `not`), ou os do C: `&&` / `\|\|` / `!`; bit a bit `&` `\|` `^` `~` `<<` `>>` | ✅ |
| Atualização | `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` (iguais ao C); `x++` / `x--` (também `++x` / `--x`); só como comando, nunca dentro de expressão | ✅ |
| Precedência | bit a bit liga mais forte que comparação (`x & 1 == 0` funciona) | ✅ |
| Funções | `fn f(x) { ... }` + `ret`, ou `f(x) = expr`; sem tipo escrito, os tipos vêm das chamadas (uma versão por combinação); com `-> tipo`, a recursão já sabe o retorno | ✅ |
| Escopo | dentro de função, variável é local por padrão: `x = ...`, `[a, b] = ...`, `ler(x)` e a variável do `para` criam uma local, mesmo que exista uma global com o mesmo nome. Para mudar uma global dentro da função, declare-a no `#decl` do topo. Posição de vetor (`dp[i] = x`) e métodos (`h.inserir(x)`) em globais funcionam sem isso, e ler uma global também. `x++` ou `x = x + 1` numa global sem `#decl` é erro (mudaria só uma cópia local) | ✅ |
| Passagem | número por valor, vetor por referência | ✅ |
| Programa | sem `main`: o topo do arquivo é o programa; `ret` encerra | ✅ |
| Entrada | `ler(a, b, v[0..n])`: da esquerda para a direita, cria o vetor se preciso | ✅ |
| Saída | `escrever(a, b)` separa por espaço e sempre termina com `\n` | ✅ |
| Mensagens de erro | no idioma do `#lang`: `erro (linha 3): índice 5 fora de 0..2` em pt, `error (line 3): index 5 outside 0..2` em en (compilação e execução) | ✅ |
| Memória | arena por execução (tudo é liberado no fim) | ✅ |
| Pilha | o programa roda com 256 MB de pilha (recursão funda) | ✅ |
| Unicode | `≤ ≥ ≠ ← ∈` como sinônimos opcionais | ✅ |
| Texto (strings) | seção "Texto" abaixo | ✅ |
| Conjunto | `conj {}` / `conj {1, 2}` (en: `set`); `s.adicionar(x)`, `s.remover(x)`, `x em s`, `s.tam()`, `para x em s` (em ordem); tipo `conj {int}`. Por dentro é o mesmo hash do mapa, só com as chaves (busca O(1), percorrer em ordem custa uma ordenação radix) | ✅ |
| Compreensão | `{x em v se x > 0}` vetor; `conj {...}` conjunto | ⏳ |
| Operações de conjunto | `a uniao b`, `a inter b`, `a menos b` (en: `union`, `inter`, `minus`) criam um conjunto novo; `inter` liga como `*`, `uniao` e `menos` como `+` | ✅ |
| Quantificadores | `para todo`, `existe` / `para algum` | ⏳ |
| `INF` | constante 10¹⁸ (folga para `INF + peso`); em vetor `: int32` vira um valor especial e volta a ser 10¹⁸ na leitura | ✅ |
| Tuplas | valor com parênteses `(d, v)`; campo com `p[0]`, `p[1]` (número fixo; também para mudar: `item[i][0] += 1`); campos inteiros, reais, strings ou outras tuplas (`(nota, nome)` ordena por nota e desempata pelo nome; `(1, (2, "ab"))` e `p[1][0]`); comparação lexicográfica; desmontar usa colchetes, como o `auto [a, b]` do C++: `[a, b] = p`, `[a, b] = (b, a)` (troca), `para [a, b] em v` | ✅ |
| Ordenar | `ordenar(v)`, `ordenar(v[i..j])`, `ordenar(v, maior)`; tuplas em ordem lexicográfica | ✅ |
| Mapa | `m = mapa {}`, `m[k]` (ausente vale 0), `k em m`, `m.remover(k)`, `para [k, x] em m` (chaves em ordem) | ✅ |
| Heap | `heap()` (menor sai primeiro), `heap(maior)`; `inserir`, `remover`, `topo` | ✅ |
| Fila | `fila()`; `enfileirar`, `desenfileirar`, `frente` | ✅ |
| Pilha | o próprio vetor: `adicionar`, `remover_ultimo` | ✅ |
| Grafo | `grafo(n)`, `grafo(n, dirigido)`; `g.aresta(u, v[, peso])`; `para v em g.vizinhos(u)`, `para [v, w] em g.vizinhos(u)` | ✅ |
| Registros | `tipo Aresta: u; v; peso` | ⏳ |
| Técnicas nativas | `prefixo`, `busca`, `mdc`, `mmc`, `potmod`, `crivo`, `conta_primos`, `conjuntos`, `fenwick`, `segtree`, `rmq` (seção abaixo) | ✅ |
| Adiado | `rep(ação, parada)`, tradutor pt↔en, compressão de coordenadas | — |

## O que já dá para escrever (marco 1)

```
#lang pt
#decl {
    dist[n];            // criado no 1º uso, quando n já foi lido
    total; media: real; // zerados
}

ler(n, m)               // vários de uma vez
ler(grid[0..n][0..m])   // cria a matriz n×m e lê
ler(v)                  // v já existe: lê todos os elementos

x = 5                   // declara
y = 2.5                 // real
v = {}                  // vetor vazio
w = {1, 2, 3}
v.adicionar(4)
u = v.remover_ultimo()
v.tam()
copia(v[1..3])

se x > 0 e nao (x == 3) {
    escrever("ok")
} senao se x < -10 {
    escrever("muito negativo")
} senao {
    escrever("outro")
}

enquanto lo < hi {
    m = (lo + hi) / 2
    // pare / continue
}

para i=0 ate n { }      // 0..n-1
para i=1 ate em n { }   // 1..n
para x em v { }         // elementos
para linha em grid { }  // linhas (vetor 1D)
para (i = n - 1; i >= 0; i--) { }   // como no C
se (x > 0) escrever(x)  // parênteses opcionais
i++

soma(v)  max(v)  min(v)  min(a, b, c)
soma(i * i para i=0 ate n)
max(x para x em v)
int(2.7)  real(3)

quadrado(x) = x * x     // função de uma linha
fn fib(k) {
    se k < 2 {
        ret k
    }
    ret fib(k - 1) + fib(k - 2)
}

escrever(a, "texto", v, grid)
```

## Texto (strings)

Uma string é um vetor de caracteres que pode ser alterado no lugar.

| Recurso | en | pt |
|---|---|---|
| literal | `"abc"` | `"abc"` |
| caractere | `'a'`, `'\n'`; `s[i]` devolve um caractere | igual |
| conta com caractere | `c - 'a'` é inteiro; `s[i] = 'a' + 2` grava o caractere | igual |
| tamanho, fatia, cópia | `s.size()`, `s[i..j]` (view), `copy(s)` | `s.tam()`, `s[i..j]`, `copia(s)` |
| comparar | `==`, `!=`, `<`, `<=`, `>`, `>=` (ordem do dicionário, por byte) | igual |
| juntar | `s + t` (nova string), `s + c` | igual |
| acrescentar | `s.add(c)`, `s.add(t)` (no fim, O(1) amortizado) | `s.adicionar(...)` |
| número → texto | `str(n)` | `texto(n)` |
| ler | `read(s)` lê uma palavra; `read_line(s)` lê a linha inteira | `ler(s)`, `ler_linha(s)` |
| escrever | `write(s)` escreve o texto; `write(c)` escreve o caractere | `escrever(...)` |
| percorrer | `for c in s` | `para c em s` |
| ordenar | `sort(s)` ordena os caracteres; vetor de strings ordena em ordem de dicionário | `ordenar(...)` |

Regras:
- `read(s)` lê uma palavra quando `s` é string: declarada com `#decl s: str` (pt: `texto`), ou já usada como string antes. Sem isso, `read(x)` continua lendo um inteiro. `read_line(s)` sempre cria string.
- String lida de uma estrutura (`s = v[i]`, `para s em v`, `[nota, nome] = p`) é um valor: mudar `s[i]` ou fazer `s.adicionar(c)` copia antes, e a guardada não muda. Por dentro, ela empresta os bytes guardados até a primeira mudança (sem cópia na leitura).
- Vetor de strings: `#decl names: str;` e depois `read(names[0..n])`, ou `names = {}` com `names.add(s)`.
- Strings guardadas em vetor, heap, fila ou como chave de mapa são **copiadas**: mudar a string depois não muda o que foi guardado. `t = s` entre variáveis compartilha os dados, como nos vetores.
- Mapa com chave string: `count = map {}`, `count[word] += 1`; percorre em ordem de dicionário.

## Técnicas nativas

Técnicas que aparecem em quase toda maratona viram parte da linguagem: menos código para escrever e nenhuma chance de errar a implementação clássica. Intervalos seguem a regra da linguagem: `[l, r)`, sem incluir `r`.

### Funções

| pt | en | O que faz | Custo |
|---|---|---|---|
| `prefixo(v)` | `prefix(v)` | vetor `P` de tamanho `n + 1` com `P[i] = v[0] + ... + v[i-1]`; soma de `v[l..r)` é `P[r] - P[l]`. Em matriz `n × m`, devolve o prefixo 2D `(n+1) × (m+1)` | O(n) |
| `busca(v, x)` | `search(v, x)` | em vetor ordenado, primeira posição com `v[i] >= x` (`v.tam()` se não houver) | O(log n) |
| `busca(v, x, maior)` | `search(v, x, greater)` | primeira posição com `v[i] > x` | O(log n) |
| `mdc(a, b)` | `gcd(a, b)` | máximo divisor comum (sempre ≥ 0) | O(log) |
| `mmc(a, b)` | `lcm(a, b)` | mínimo múltiplo comum | O(log) |
| `potmod(b, e, m)` | `powmod(b, e, m)` | `b^e mod m` por exponenciação rápida (resultado em `[0, m)`) | O(log e) |
| `crivo(n)` | `sieve(n)` | vetor com os primos `≤ n`, em ordem | O(n log log n) |
| `conta_primos(n)` | `count_primes(n)` | quantos primos `≤ n`, sem montar o vetor (Lucy_Hedgehog): n = 10¹¹ em ~0,5 s e O(√n) de memória | O(n^(3/4)) |

### Estruturas

| pt | en | Operações | Custo |
|---|---|---|---|
| `d = conjuntos(n)` | `d = dsu(n)` | `d.unir(a, b)` (devolve 1 se juntou dois grupos), `d.achar(a)`, `d.juntos(a, b)`, `d.tam(a)` (tamanho do grupo de `a`), `d.grupos()` | ≈O(1) (union by size + path halving) |
| `f = fenwick(n)` | `f = fenwick(n)` | `f.somar(i, x)` (soma `x` na posição `i`), `f.soma(l, r)` (soma de `[l, r)`) | O(log n) |
| `s = segtree(v, min)` | `s = segtree(v, min)` | também `max` e `soma`/`sum`; `s.consulta(l, r)` (resultado em `[l, r)`), `s.mudar(i, x)` (en: `s.update(i, x)`; troca o valor da posição `i`) | O(log n) |
| `r = rmq(v, min)` | `r = rmq(v, min)` | também `max`; `r.consulta(l, r)`; vetor que não muda (sparse table) | O(1) consulta, O(n log n) construção |

Detalhes:
- Os nomes são nativos: um programa não pode ter função própria chamada `busca`, `prefixo` etc.
- `x.tam()` (`x.size()`) funciona em todas: número de elementos (`n`).
- Fenwick e segtree guardam inteiros de 64 bits ou reais, conforme os valores usados.
- `consulta(l, r)` com intervalo vazio (`l >= r`) é erro com a linha (sem `-O`).
- Os vetores devolvidos por `prefixo` e `crivo` são sempre de 64 bits: a análise de faixa não os estreita.

Exemplo (Kruskal):

```
#lang pt
ler(n, m)
arestas = {}
para i=0 ate m {
    ler(u, v, w)
    arestas.adicionar((w, u, v))
}
ordenar(arestas)
d = conjuntos(n)
total = 0
para [w, u, v] em arestas {
    se d.unir(u, v) {
        total += w
    }
}
escrever(total)
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
| `uniao` | `union` | | `conj` | `set` |
| `inter` | `inter` | | `tipo` ⏳ | `type` |
| `menos` | `minus` | | | |

As palavras marcadas com ⏳ já são reservadas, para que nenhum programa precise mudar quando elas entrarem.

## Estrutura do compilador

| Arquivo | Papel |
|---|---|
| `src/lex.c` | tokens, indentação → INDENT/DEDENT, palavras por idioma |
| `src/messages.c` | mensagens de erro em inglês (o texto em português é a chave); as do runtime ficam em `rt/core.h` |
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

- **Idiomas configuráveis**: hoje pt e en estão em tabelas dentro do código (`src/language.c`, e as mensagens em `src/messages.c` e `rt/core.h`). A ideia é tirar as palavras-chave, os nomes nativos e os métodos para arquivos de idioma (um por idioma), para que qualquer pessoa traduza a linguagem só criando uma configuração nova (`#lang es`, `#lang fr`...). Importante quando o projeto virar open source.
- **Tipagem forte opcional (resto)**: já existe no `#decl`, nos parâmetros e no retorno, para todos os tipos. Falta só o tipo na primeira atribuição (`v: {int} = {}`), se fizer falta.
