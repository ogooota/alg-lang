#!/usr/bin/env python3
"""Gera docs/guia.html (inglês + português) a partir dos exemplos testados em docs/examples/."""
import html
import os

HERE = os.path.dirname(os.path.abspath(__file__))
EXAMPLES = os.path.join(HERE, "examples")


def read(*parts):
    with open(os.path.join(EXAMPLES, *parts)) as f:
        return f.read().rstrip("\n")


def lines(code):
    return sum(1 for line in code.split("\n") if line.strip())


def code_block(code, lang):
    return f'<pre><code class="lang-{lang}">{html.escape(code, quote=False)}</code></pre>'


def t(en, pt):
    return {"en": en, "pt": pt}


PROBLEMS = [
    dict(slug="prefix", tag=t("prefix sums · O(n + q)", "soma de prefixos · O(n + q)"),
         title=t("Range sums", "Soma de intervalo"),
         text=t("Given <var>n</var> numbers, answer <var>q</var> questions \"what is the sum of <var>a[l..r]</var>?\" (1-based). Input <code>5 3 / 1 2 3 4 5 / 1 5 / 2 3 / 4 4</code> → <code>15 5 4</code>.",
                "Dado um vetor de <var>n</var> números, responda <var>q</var> perguntas \"quanto dá a soma de <var>a[l..r]</var>?\" (posições a partir de 1). Entrada <code>5 3 / 1 2 3 4 5 / 1 5 / 2 3 / 4 4</code> → <code>15 5 4</code>."),
         notes=t(["<code>#decl pre[n + 1]</code> comes before <code>n</code> is read: the array is only created (zeroed) when first used.",
                  "Every integer is 64-bit, so the sum never overflows the way an <code>int</code> would."],
                 ["<code>#decl pre[n + 1]</code> aparece antes de <code>n</code> ser lido: o vetor só é criado (zerado) no primeiro uso.",
                  "Todo inteiro já é de 64 bits, então a soma não estoura como estouraria com <code>int</code>."])),
    dict(slug="knapsack", tag=t("dynamic programming · O(n · W)", "programação dinâmica · O(n · W)"),
         title=t("0/1 knapsack", "Mochila 0/1"),
         text=t("<var>n</var> items with weight and value, a bag of capacity <var>W</var>: what is the best total value? Input <code>4 7 / 1 1 / 3 4 / 4 5 / 5 7</code> → <code>9</code>.",
                "<var>n</var> itens com peso e valor, mochila com capacidade <var>W</var>: qual o maior valor sem passar do peso? Entrada <code>4 7 / 1 1 / 3 4 / 4 5 / 5 7</code> → <code>9</code>."),
         notes=t(["Counting down uses the C-style <code>for (;;)</code>, which alg also accepts; parentheses are optional on <code>if</code>, <code>while</code> and <code>for</code>.",
                  "With <code>-O</code> this loop runs in 0.56× the C++ time in the project benchmark (the generated C vectorizes)."],
                 ["Para contar de trás para frente, use o <code>para (;;)</code> do C, que o alg também aceita; os parênteses são opcionais em <code>se</code>, <code>enquanto</code> e <code>para</code>.",
                  "Com <code>-O</code>, este laço roda em 0,56× o tempo do C++ no benchmark do projeto (o C gerado é vetorizado)."])),
    dict(slug="activities", tag=t("greedy + sorting · O(n log n)", "guloso + ordenação · O(n log n)"),
         title=t("Most non-overlapping activities", "Máximo de atividades sem sobreposição"),
         text=t("Each activity has a start and an end. How many can you do without overlaps? Sort by end time and take the next one that starts after the last chosen. Input <code>4 / 1 3 / 2 5 / 3 9 / 6 8</code> → <code>2</code>.",
                "Cada atividade tem início e fim. Quantas dá para fazer sem sobreposição? Ordene pelo fim e pegue a próxima que começa depois da última escolhida. Entrada <code>4 / 1 3 / 2 5 / 3 9 / 6 8</code> → <code>2</code>."),
         notes=t(["Tuples compare field by field, like <code>pair</code>: <code>(end, start)</code> sorts by end, then by start.",
                  "<code>sort</code> uses radix sort for integers and introsort for tuples and reals: 0.46× the time of <code>std::sort</code> in the benchmark."],
                 ["Tuplas comparam campo a campo, como <code>pair</code>: <code>(fim, inicio)</code> ordena pelo fim e desempata pelo início.",
                  "<code>ordenar</code> usa radix sort para inteiros e introsort para tuplas e reais: 0,46× o tempo do <code>std::sort</code> no benchmark."])),
    dict(slug="truck", tag=t("binary search on the answer · O(n log S)", "busca binária na resposta · O(n log S)"),
         title=t("Smallest truck capacity", "Menor capacidade de caminhão"),
         text=t("Packages, in order, must go in at most <var>k</var> trips. What is the smallest truck capacity that works? Binary search on the capacity. Input <code>5 3 / 4 2 7 3 5</code> → <code>8</code>.",
                "Pacotes em ordem precisam ir em no máximo <var>k</var> viagens. Qual a menor capacidade de caminhão que dá conta? Busca binária na capacidade. Entrada <code>5 3 / 4 2 7 3 5</code> → <code>8</code>."),
         notes=t(["Functions do not declare parameter types: the compiler infers them from the calls and emits one C version per combination of types.",
                  "Arrays are passed by reference, never copied: C++'s <code>const vector&lt;long long&gt; &amp;</code> becomes just <code>weights</code>."],
                 ["A função não declara o tipo dos parâmetros: o compilador descobre pelas chamadas e gera uma versão em C para cada combinação de tipos.",
                  "Vetores passam por referência, sem cópia: o <code>const vector&lt;long long&gt; &amp;</code> do C++ vira só <code>pesos</code>."])),
    dict(slug="frequency", tag=t("hashing · O(n) + sorted keys", "hash · O(n) + chaves ordenadas"),
         title=t("How often each value appears", "Frequência de cada valor"),
         text=t("Read <var>n</var> numbers and print each distinct value with its count, in increasing order. Input <code>6 / 5 1 5 3 1 5</code> → <code>1 2 / 3 1 / 5 3</code>.",
                "Leia <var>n</var> números e mostre cada valor distinto com quantas vezes aparece, em ordem crescente. Entrada <code>6 / 5 1 5 3 1 5</code> → <code>1 2 / 3 1 / 5 3</code>."),
         notes=t(["A map is a hash table (fast like <code>unordered_map</code>) that iterates its keys in order (like <code>map</code>). You do not have to choose.",
                  "The hash seed changes on every run, so anti-hash tests built against <code>unordered_map</code> do not work here."],
                 ["O mapa é um hash (rápido como <code>unordered_map</code>) e percorre as chaves em ordem (como <code>map</code>). Você não precisa escolher.",
                  "A semente do hash muda a cada execução: entradas feitas para derrubar <code>unordered_map</code> não funcionam aqui."])),
    dict(slug="bfs", tag=t("graph · BFS · O(n + m)", "grafo · BFS · O(n + m)"),
         title=t("Fewest roads", "Menor número de estradas"),
         text=t("Cities 1..<var>n</var> and <var>m</var> two-way roads. What is the fewest number of roads from city 1 to city <var>n</var>? Input <code>5 5 / 1 2 / 2 3 / 1 4 / 4 5 / 3 5</code> → <code>2</code>.",
                "Cidades 1..<var>n</var> e <var>m</var> estradas de mão dupla. Quantas estradas no mínimo da cidade 1 até a <var>n</var>? Entrada <code>5 5 / 1 2 / 2 3 / 1 4 / 4 5 / 3 5</code> → <code>2</code>."),
         notes=t(["<code>graph(n)</code> is undirected: <code>edge(u, v)</code> links both ways. Forgetting the reverse edge, a classic bug, cannot happen.",
                  "The graph becomes a compact adjacency array (CSR) on first query; BFS ran in 0.45× the time of the <code>vector&lt;vector&gt;</code> version."],
                 ["<code>grafo(n)</code> é não dirigido: <code>aresta(u, v)</code> já liga os dois lados. Esquecer a volta, um bug clássico, não acontece.",
                  "O grafo vira uma adjacência compacta (CSR) na primeira consulta; o BFS levou 0,45× o tempo da versão com <code>vector&lt;vector&gt;</code>."])),
    dict(slug="dijkstra", tag=t("weighted shortest path · O((n + m) log m)", "menor caminho com pesos · O((n + m) log m)"),
         title=t("Dijkstra", "Dijkstra"),
         text=t("Directed graph with positive weights: shortest distance from city 1 to every city. Input <code>3 4 / 1 2 6 / 1 3 2 / 3 2 3 / 2 3 4</code> → <code>0 5 2</code>.",
                "Grafo dirigido com pesos positivos: menor distância da cidade 1 até cada cidade. Entrada <code>3 4 / 1 2 6 / 1 3 2 / 3 2 3 / 2 3 4</code> → <code>0 5 2</code>."),
         notes=t(["<code>heap()</code> pops the smallest first: no <code>greater&lt;&gt;</code>, no negated keys. For the largest first, <code>heap(greater)</code>.",
                  "<code>INF</code> is 10<sup>18</sup>, leaving room for <code>INF + w</code>. In an <code>: int32</code> array the compiler stores it specially and it still reads back as INF.",
                  "<code>write(dist)</code> prints the whole array separated by spaces."],
                 ["<code>heap()</code> já tira o menor primeiro: nada de <code>greater&lt;&gt;</code> nem de inverter sinal. Para o maior primeiro, <code>heap(maior)</code>.",
                  "<code>INF</code> vale 10<sup>18</sup>, com folga para <code>INF + w</code>. Num vetor <code>: int32</code>, o compilador guarda INF de um jeito especial e ele continua valendo INF quando lido.",
                  "<code>escrever(dist)</code> escreve o vetor inteiro separado por espaço."])),
    dict(slug="kruskal", tag=t("union-find · O(m log m)", "union-find · O(m log m)"),
         title=t("Minimum spanning tree (Kruskal)", "Árvore geradora mínima (Kruskal)"),
         text=t("Connect all <var>n</var> cities with roads of minimum total cost. Sort the roads by cost and take each one that joins two separate groups. Input <code>4 5 / 1 2 4 / 2 3 1 / 1 3 3 / 3 4 2 / 2 4 5</code> → <code>6</code>.",
                "Ligue as <var>n</var> cidades com estradas de custo total mínimo. Ordene as estradas pelo custo e pegue cada uma que junta dois grupos separados. Entrada <code>4 5 / 1 2 4 / 2 3 1 / 1 3 3 / 3 4 2 / 2 4 5</code> → <code>6</code>."),
         notes=t(["<code>dsu(n)</code> is built in (union by size + path halving): no struct to copy from your notebook.",
                  "<code>d.merge(u, v)</code> returns 1 when it joined two groups, so the loop body is a single <code>if</code>, and single-statement bodies need no braces."],
                 ["<code>conjuntos(n)</code> já vem pronto (união por tamanho + compressão de caminho): nada de copiar a struct do caderno.",
                  "<code>d.unir(u, v)</code> devolve 1 quando juntou dois grupos, então o corpo do laço é um único <code>se</code>, e corpo de um comando só dispensa as chaves."])),
]

TECHNIQUES = [
    ("prefix(v)", "prefixo(v)", t("array <code>P</code> of size <code>n + 1</code> with <code>P[i] = v[0] + ... + v[i-1]</code>; the sum of <code>v[l..r)</code> is <code>P[r] - P[l]</code>. On a matrix, the 2D prefix", "vetor <code>P</code> de tamanho <code>n + 1</code> com <code>P[i] = v[0] + ... + v[i-1]</code>; a soma de <code>v[l..r)</code> é <code>P[r] - P[l]</code>. Em matriz, o prefixo 2D"), "O(n)"),
    ("search(v, x)", "busca(v, x)", t("first index with <code>v[i] &gt;= x</code> in a sorted array; <code>search(v, x, greater)</code> for <code>v[i] &gt; x</code>", "primeira posição com <code>v[i] &gt;= x</code> em vetor ordenado; <code>busca(v, x, maior)</code> para <code>v[i] &gt; x</code>"), "O(log n)"),
    ("gcd, lcm", "mdc, mmc", t("greatest common divisor and least common multiple", "máximo divisor comum e mínimo múltiplo comum"), "O(log)"),
    ("powmod(b, e, m)", "potmod(b, e, m)", t("<code>b<sup>e</sup> mod m</code> by fast exponentiation, no overflow", "<code>b<sup>e</sup> mod m</code> por exponenciação rápida, sem estouro"), "O(log e)"),
    ("sieve(n)", "crivo(n)", t("array of the primes up to <code>n</code>", "vetor com os primos até <code>n</code>"), "O(n log log n)"),
    ("dsu(n)", "conjuntos(n)", t("union-find: <code>merge(a, b)</code>, <code>find(a)</code>, <code>same(a, b)</code>, <code>size(a)</code>, <code>groups()</code>", "union-find: <code>unir(a, b)</code>, <code>achar(a)</code>, <code>juntos(a, b)</code>, <code>tam(a)</code>, <code>grupos()</code>"), "≈O(1)"),
    ("fenwick(n)", "fenwick(n)", t("<code>add(i, x)</code> and <code>sum(l, r)</code> over <code>[l, r)</code>", "<code>somar(i, x)</code> e <code>soma(l, r)</code> em <code>[l, r)</code>"), "O(log n)"),
    ("segtree(v, min)", "segtree(v, min)", t("also <code>max</code> and <code>sum</code>; <code>query(l, r)</code> and <code>update(i, x)</code>", "também <code>max</code> e <code>soma</code>; <code>consulta(l, r)</code> e <code>mudar(i, x)</code>"), "O(log n)"),
    ("rmq(v, min)", "rmq(v, min)", t("sparse table for an array that does not change; <code>query(l, r)</code>", "sparse table para vetor que não muda; <code>consulta(l, r)</code>"), "O(1)"),
]

BASICS = {
    "en": [
        ("Input and output", "read(n, m)          // two integers\nread(v[0..n])       // creates v, reads n values\nread(grid[0..n][0..m])\nwrite(a, b, v)      // space-separated, then newline"),
        ("Loops and conditions", "for i=0 to n { }     // 0, 1, ..., n-1\nfor i=1 thr n { }    // 1, 2, ..., n\nfor x in v { }\nwhile lo < hi { }\nif x > 0 and not done {\n} else if x == 0 {\n} else {\n}"),
        ("Functions", "square(x) = x * x\n\nfn fib(k) {\n    if k < 2 {\n        ret k\n    }\n    ret fib(k - 1) + fib(k - 2)\n}"),
        ("Updating values", "x += 5      // also -= *= /= %= &= |= ^= <<= >>=\ni++\ni--\n[a, b] = (b, a)   // swap with a tuple"),
        ("Arrays declared up front", "#decl {\n    dp[n + 1][m + 1]: int32;\n    seen[n];\n}\nread(n, m)    // n may come later"),
        ("Built-in structures", "v = {}         v.add(x)\nh = heap()     h.push((d, u))\nq = queue()    q.push(u)\nm = map {}     m[key] += 1\ng = graph(n)   g.edge(u, v, w)\nsort(v)        sort(v, greater)"),
    ],
    "pt": [
        ("Entrada e saída", "ler(n, m)          // dois inteiros\nler(v[0..n])       // cria v e lê n valores\nler(grade[0..n][0..m])\nescrever(a, b, v)  // separa por espaço, quebra a linha"),
        ("Laços e condições", "para i=0 ate n { }      // 0, 1, ..., n-1\npara i=1 ate em n { }   // 1, 2, ..., n\npara x em v { }\nenquanto lo < hi { }\nse x > 0 e nao feito {\n} senao se x == 0 {\n} senao {\n}"),
        ("Funções", "quadrado(x) = x * x\n\nfn fib(k) {\n    se k < 2 {\n        ret k\n    }\n    ret fib(k - 1) + fib(k - 2)\n}"),
        ("Atualizar valores", "x += 5      // também -= *= /= %= &= |= ^= <<= >>=\ni++\ni--\n[a, b] = (b, a)   // troca com tupla"),
        ("Vetores declarados antes", "#decl {\n    dp[n + 1][m + 1]: int32;\n    visitado[n];\n}\nler(n, m)     // n pode vir depois"),
        ("Estruturas prontas", "v = {}         v.adicionar(x)\nh = heap()     h.inserir((d, u))\nf = fila()     f.enfileirar(u)\nm = mapa {}    m[chave] += 1\ng = grafo(n)   g.aresta(u, v, peso)\nordenar(v)     ordenar(v, maior)"),
    ],
}

TRANSLATION = [
    ("<code>cin &gt;&gt; n;</code> + <code>vector&lt;ll&gt; a(n)</code> + loop", "<code>read(n, a[0..n])</code>", "<code>ler(n, a[0..n])</code>", t("reads and creates the array", "lê e cria o vetor")),
    ("<code>cout &lt;&lt; a &lt;&lt; ' ' &lt;&lt; b &lt;&lt; '\\n';</code>", "<code>write(a, b)</code>", "<code>escrever(a, b)</code>", t("buffered output", "saída bufferizada")),
    ("<code>for (int i = 0; i &lt; n; i++) {</code>", "<code>for i=0 to n {</code>", "<code>para i=0 ate n {</code>", t("<code>to</code> excludes the end", "<code>ate</code> não inclui o fim")),
    ("<code>for (int i = 1; i &lt;= n; i++) {</code>", "<code>for i=1 thr n {</code>", "<code>para i=1 ate em n {</code>", t("<code>thr</code> includes it", "<code>ate em</code> inclui")),
    ("<code>for (auto x : v) {</code>", "<code>for x in v {</code>", "<code>para x em v {</code>", t("", "")),
    ("<code>for (int i = n - 1; i &gt;= 0; i--)</code>", "<code>for (i = n - 1; i &gt;= 0; i--)</code>", "<code>para (i = n - 1; i &gt;= 0; i--)</code>", t("the C form also works", "a forma do C também vale")),
    ("<code>long long</code>", "—", "—", t("every integer is 64-bit", "todo inteiro é de 64 bits")),
    ("global <code>int timer;</code> changed in a function", "<code>#decl timer</code> at the top", "<code>#decl timer</code> no topo", t("other names assigned in a function are local", "outros nomes atribuídos na função são locais")),
    ("<code>int dp[N][M]</code> (global)", "<code>#decl dp[n][m]: int32;</code>", "<code>#decl dp[n][m]: int32;</code>", t("real size, not the maximum", "tamanho real, não o máximo")),
    ("<code>i++</code>, <code>i--</code>", "<code>i++</code>, <code>i--</code>", "<code>i++</code>, <code>i--</code>", t("statements only", "só como comando")),
    ("<code>pair&lt;ll, int&gt;</code>", "<code>(d, u)</code>", "<code>(d, u)</code>", t("tuples of any size", "tupla de qualquer tamanho")),
    ("<code>auto [d, u] = p;</code>", "<code>[d, u] = p</code>", "<code>[d, u] = p</code>", t("", "")),
    ("<code>sort(v.begin(), v.end())</code>", "<code>sort(v)</code>", "<code>ordenar(v)</code>", t("<code>sort(v, greater)</code> for descending", "<code>ordenar(v, maior)</code> decrescente")),
    ("<code>priority_queue&lt;T, vector&lt;T&gt;, greater&lt;&gt;&gt;</code>", "<code>heap()</code>", "<code>heap()</code>", t("smallest first", "menor primeiro")),
    ("<code>queue&lt;int&gt;</code>", "<code>queue()</code>", "<code>fila()</code>", t("<code>push</code> / <code>pop</code>", "<code>enfileirar</code> / <code>desenfileirar</code>")),
    ("<code>map</code> / <code>unordered_map</code>", "<code>map {}</code>", "<code>mapa {}</code>", t("missing key reads as 0", "chave ausente vale 0")),
    ("<code>m.count(k)</code>", "<code>k in m</code>", "<code>k em m</code>", t("", "")),
    ("<code>vector&lt;vector&lt;int&gt;&gt; g(n)</code> + both <code>push_back</code>s", "<code>g = graph(n)</code> + <code>g.edge(u, v)</code>", "<code>g = grafo(n)</code> + <code>g.aresta(u, v)</code>", t("<code>graph(n, directed)</code> for one-way", "<code>grafo(n, dirigido)</code> para mão única")),
    ("<code>accumulate</code>, <code>*max_element</code>", "<code>sum(v)</code>, <code>max(v)</code>", "<code>soma(v)</code>, <code>max(v)</code>", t("also <code>sum(i*i for i=0 to n)</code>", "também <code>soma(i*i para i=0 ate n)</code>")),
    ("<code>&amp;&amp;</code>, <code>||</code>, <code>!</code>", "<code>and</code>, <code>or</code>, <code>not</code>", "<code>e</code>, <code>ou</code>, <code>nao</code>", t("<code>&amp; | ^ ~ &lt;&lt; &gt;&gt;</code> stay bitwise", "<code>&amp; | ^ ~ &lt;&lt; &gt;&gt;</code> continuam bit a bit")),
]

PITFALLS = [
    t(("<code>int</code> overflows on 10<sup>5</sup> · 10<sup>5</sup>", "integers are 64-bit; arrays use fewer bytes only when the compiler proves the values fit"),
      ("<code>int</code> estoura em 10<sup>5</sup> · 10<sup>5</sup>", "inteiros são de 64 bits; vetores usam menos bytes só quando o compilador prova que os valores cabem")),
    t(("<code>-7 % 2 == -1</code> breaks modular arithmetic", "<code>-7 % 2 == 1</code> and <code>-7 / 2 == -4</code>: the remainder is never negative"),
      ("<code>-7 % 2 == -1</code> quebra aritmética modular", "<code>-7 % 2 == 1</code> e <code>-7 / 2 == -4</code>: o resto nunca é negativo")),
    t(("<code>INF + w</code> overflows when <code>INF = LLONG_MAX</code>", "<code>INF</code> is already 10<sup>18</sup>"),
      ("<code>INF + w</code> estoura quando <code>INF = LLONG_MAX</code>", "<code>INF</code> já vale 10<sup>18</sup>")),
    t(("<code>x &amp; 1 == 0</code> parses as <code>x &amp; (1 == 0)</code>", "bitwise binds tighter than comparison: it means what it says"),
      ("<code>x &amp; 1 == 0</code> é lido como <code>x &amp; (1 == 0)</code>", "bit a bit liga mais forte que comparação: funciona como se lê")),
    t(("recursive DFS on 10<sup>5</sup> nodes blows the 8 MB stack", "programs run with a 256 MB stack"),
      ("DFS recursiva com 10<sup>5</sup> nós estoura a pilha de 8 MB", "o programa roda com 256 MB de pilha")),
    t(("<code>cin</code> without <code>sync_with_stdio(false)</code> gets TLE", "input is already fast (2× optimized <code>cin</code>)"),
      ("<code>cin</code> sem <code>sync_with_stdio(false)</code> leva TLE", "a leitura já é rápida (2× mais que <code>cin</code> otimizado)")),
    t(("<code>a[i++] = i</code>: unspecified evaluation order", "<code>++</code> and <code>+=</code> are statements, never part of an expression"),
      ("<code>a[i++] = i</code>: ordem de avaliação indefinida", "<code>++</code> e <code>+=</code> são comandos, nunca parte de expressão")),
    t(("forgetting the reverse edge of an undirected graph", "<code>graph(n)</code> is undirected by default"),
      ("esquecer a aresta de volta no grafo não dirigido", "<code>grafo(n)</code> já é não dirigido")),
    t(("<code>unordered_map</code> killed by an anti-hash test", "the hash seed is random on every run"),
      ("<code>unordered_map</code> derrubado por entrada adversária", "semente aleatória no hash a cada execução")),
    t(("out-of-bounds index means garbage or a segfault", "without <code>-O</code>, a runtime error with the line number"),
      ("índice fora do vetor vira lixo ou segfault", "sem <code>-O</code>, erro com a linha")),
]

PERF = [
    (t("Dijkstra, 10<sup>5</sup> vertices, 10<sup>6</sup> edges", "Dijkstra, 10<sup>5</sup> vértices, 10<sup>6</sup> arestas"), "0.170", "0.468", "0.36"),
    (t("BFS, 10<sup>6</sup> vertices, 2·10<sup>6</sup> edges", "BFS, 10<sup>6</sup> vértices, 2·10<sup>6</sup> arestas"), "0.675", "1.402", "0.45"),
    (t("sort 10<sup>6</sup> integers", "ordenar 10<sup>6</sup> inteiros"), "0.085", "0.187", "0.46"),
    (t("prefix sums, 10<sup>6</sup> queries", "soma de prefixos, 10<sup>6</sup> consultas"), "0.230", "0.450", "0.50"),
    (t("knapsack, 10<sup>8</sup> operations", "mochila, 10<sup>8</sup> operações"), "0.045", "0.077", "0.57"),
    (t("count 10<sup>6</sup> values (map)", "contar 10<sup>6</sup> valores (mapa)"), "0.186", "0.297", "0.62"),
    (t("LCS 4000 × 4000", "LCS 4000 × 4000"), "0.072", "0.074", "0.96"),
    (t("sieve up to 2·10<sup>7</sup>", "crivo até 2·10<sup>7</sup>"), "0.181", "0.186", "0.99"),
    (t("recursive fib(38)", "fib(38) recursivo"), "0.095", "0.093", "1.01"),
    (t("union-find, 10<sup>6</sup> operations", "union-find, 10<sup>6</sup> operações"), "0.163", "0.324", "0.50"),
    (t("Fenwick, 10<sup>6</sup> updates and sums", "Fenwick, 10<sup>6</sup> atualizações e somas"), "0.168", "0.319", "0.53"),
    (t("segment tree (min), 10<sup>6</sup> updates and queries", "segtree (min), 10<sup>6</sup> atualizações e consultas"), "0.258", "0.462", "0.56"),
    (t("sparse table, 10<sup>6</sup> range minimums", "sparse table, 10<sup>6</sup> mínimos de intervalo"), "0.247", "0.435", "0.57"),
    (t("sieve to 2·10<sup>7</sup> (built-in)", "crivo até 2·10<sup>7</sup> (nativo)"), "0.194", "0.193", "1.01"),
]

UI = {
    "en": dict(
        eyebrow="a language for competitive programming",
        lead="alg reads like textbook pseudocode and compiles to C. You write the algorithm; it takes care of fast input, 64-bit integers, negative remainders and picking the right data structure. Every example below shows the same solution in C++ next to it, so you can see what you no longer have to write.",
        toc=["Run", "Basics", "Range sums", "Knapsack", "Greedy", "Binary search", "Counting", "BFS", "Dijkstra", "Kruskal", "Techniques", "C++ → alg", "Pitfalls", "Speed"],
        run_eyebrow="first", run_title="Running a program",
        run_text="Files use the <code>.alg</code> extension. The first line picks the keyword language: <code>#lang en</code> or <code>#lang pt</code> (Portuguese).",
        run_cmds="./alg sol.alg                 # compile and run; type the input\n./alg sol.alg < input.txt     # or pipe a file\n./alg -O -o sol sol.alg       # fast executable, no checks (to submit)\n./alg -O --emit-c sol.alg     # print the generated C (judges accept it as C)",
        run_after="Without <code>-O</code>, an out-of-bounds index, a division by zero or popping an empty structure stops with the line number: <code>erro (linha 12): índice 7 fora de 0..5</code> (messages are in Portuguese for now). With <code>-O</code> the checks go away and the code runs as fast as C.",
        basics_eyebrow="syntax", basics_title="The basics on one screen",
        basics_text="Blocks use braces, and a single-statement body needs none, as in C. There are no types to declare: <code>x = 5</code> creates a 64-bit integer. Comments start with <code>//</code>, a newline ends a statement and a trailing <code>;</code> is optional.",
        problems_title="Contest problems, side by side",
        problems_text="Every alg solution here was compiled and tested against the sample input, and gives the same output as the C++ version, which is written the way contestants usually write it (with <code>sync_with_stdio(false)</code>).",
        lines="lines", translation_eyebrow="quick reference", translation_title="C++ → alg",
        col_cpp="C++", col_alg="alg", col_note="Note",
        pitfalls_eyebrow="bugs that cannot happen", pitfalls_title="C++ pitfalls alg avoids", col_in_cpp="In C++", col_in_alg="In alg",
        perf_eyebrow="measured", perf_title="Speed",
        perf_text="alg with <code>-O</code> against C++ <code>-O2</code> with optimized <code>cin</code>, same input, best of 5 runs. Below 1 means alg was faster.",
        col_problem="Problem", col_ratio="Ratio",
        perf_callout="Part of the advantage on the structure problems comes from input reading. Against C++ with a hand-written fast reader, alg still wins on sorting (0.56×), Dijkstra (0.55×), BFS (0.53×) and maps (0.69×), and is within 10% on the technique structures (union-find 0.98×, Fenwick 1.01×, segment tree 1.09×, sparse table 0.90×), which contestants usually code by hand.",
        footer="Full specification (in Portuguese) in <code>docs/linguagem.md</code>. To benchmark on your machine: <code>python3 bench/gen_inputs.py && python3 bench/run.py</code>.",
        switch="Português", decimal=".",
        tech_eyebrow="built in", tech_title="Contest techniques as functions",
        tech_text="Techniques that show up in almost every contest are part of the language, so there is no template to paste and no classic implementation to get wrong. Ranges follow the language rule: <code>[l, r)</code>.",
        col_call="Call", col_does="What it does", col_cost="Cost",
    ),
    "pt": dict(
        eyebrow="linguagem para maratona de programação",
        lead="O alg se lê como o pseudocódigo do livro e compila para C. Você escreve o algoritmo; ele cuida da leitura rápida da entrada, do <code>long long</code>, do resto negativo e da estrutura de dados certa. Cada exemplo abaixo traz a mesma solução em C++ ao lado, para você ver o que deixa de escrever.",
        toc=["Rodar", "O básico", "Soma de intervalo", "Mochila", "Guloso", "Busca binária", "Contagem", "BFS", "Dijkstra", "Kruskal", "Técnicas", "C++ → alg", "Armadilhas", "Desempenho"],
        run_eyebrow="antes de tudo", run_title="Rodar um programa",
        run_text="O arquivo tem extensão <code>.alg</code>. A primeira linha diz o idioma das palavras-chave: <code>#lang pt</code> ou <code>#lang en</code>.",
        run_cmds="./alg sol.alg                 # compila e roda; digite a entrada\n./alg sol.alg < entrada.txt   # ou mande um arquivo\n./alg -O -o sol sol.alg       # executável rápido, sem checagens (para submeter)\n./alg -O --emit-c sol.alg     # mostra o C gerado (dá para enviar ao juiz como C)",
        run_after="Sem <code>-O</code>, índice fora do vetor, divisão por zero e estrutura vazia viram erro com a linha: <code>erro (linha 12): índice 7 fora de 0..5</code>. Com <code>-O</code> essas checagens saem e o código fica tão rápido quanto C.",
        basics_eyebrow="sintaxe", basics_title="O básico em uma tela",
        basics_text="Blocos usam chaves, e um corpo de um comando só dispensa as chaves, como em C. Não há tipos para declarar: <code>x = 5</code> cria um inteiro de 64 bits. Comentário é <code>//</code>, o fim da linha termina o comando e o <code>;</code> no fim é opcional.",
        problems_title="Problemas de maratona, lado a lado",
        problems_text="Cada solução em alg foi compilada e testada com a entrada de exemplo e dá a mesma saída da versão em C++, escrita como um competidor escreve no dia a dia (com <code>sync_with_stdio(false)</code>).",
        lines="linhas", translation_eyebrow="referência rápida", translation_title="Tradução C++ → alg",
        col_cpp="C++", col_alg="alg", col_note="Observação",
        pitfalls_eyebrow="bugs que não acontecem", pitfalls_title="Armadilhas do C++ que o alg evita", col_in_cpp="Em C++", col_in_alg="No alg",
        perf_eyebrow="medido", perf_title="Desempenho",
        perf_text="alg com <code>-O</code> contra C++ <code>-O2</code> com <code>cin</code> otimizado, mesma entrada, melhor de 5 execuções. Menor que 1 significa que o alg foi mais rápido.",
        col_problem="Problema", col_ratio="Razão",
        perf_callout="Nos problemas de estrutura, parte da vantagem vem da leitura de entrada. Contra C++ com um leitor rápido feito à mão, o alg continua na frente em ordenar (0,56×), Dijkstra (0,55×), BFS (0,53×) e mapa (0,69×), e fica a menos de 10% nas estruturas de técnica (union-find 0,98×, Fenwick 1,01×, segtree 1,09×, sparse table 0,90×), que o competidor costuma escrever à mão.",
        footer="Especificação completa em <code>docs/linguagem.md</code>. Para medir no seu computador: <code>python3 bench/gen_inputs.py && python3 bench/run.py</code>.",
        switch="English", decimal=",",
        tech_eyebrow="nativo", tech_title="Técnicas de maratona como funções",
        tech_text="Técnicas que aparecem em quase toda maratona fazem parte da linguagem: nada de colar template nem de errar a implementação clássica. Intervalos seguem a regra da linguagem: <code>[l, r)</code>.",
        col_call="Chamada", col_does="O que faz", col_cost="Custo",
    ),
}

ANCHORS = ["run", "basics", "prefix", "knapsack", "activities", "truck", "frequency", "bfs", "dijkstra", "kruskal", "techniques", "translation", "pitfalls", "speed"]


def number(value, lang):
    return value.replace(".", UI[lang]["decimal"])


def build(lang):
    ui = UI[lang]
    out = [f'<div class="lang" data-lang="{lang}" lang="{lang}"' + (" hidden" if lang == "pt" else "") + ">"]
    out.append(f'<header><div class="eyebrow">{ui["eyebrow"]}</div><h1>alg</h1><p class="lead prose">{ui["lead"]}</p>')
    out.append('<nav class="toc">' + "".join(f'<a href="#{lang}-{a}">{n}</a>' for a, n in zip(ANCHORS, ui["toc"])) + "</nav></header>")
    out.append(f'<section id="{lang}-run" class="prose"><div class="eyebrow">{ui["run_eyebrow"]}</div><h2>{ui["run_title"]}</h2><p>{ui["run_text"]}</p>')
    out.append(code_block(ui["run_cmds"], "sh") + f'<p>{ui["run_after"]}</p></section>')
    out.append(f'<section id="{lang}-basics"><div class="eyebrow">{ui["basics_eyebrow"]}</div><h2>{ui["basics_title"]}</h2><p class="prose">{ui["basics_text"]}</p><div class="grid-basics">')
    for title, code in BASICS[lang]:
        out.append(f"<div><h3>{title}</h3>{code_block(code, 'alg')}</div>")
    out.append("</div></section>")
    out.append(f'<h2>{ui["problems_title"]}</h2><p class="prose">{ui["problems_text"]}</p>')
    for p in PROBLEMS:
        alg = read(lang, p["slug"] + ".alg")
        cpp = read("cpp", p["slug"] + ".cpp")
        out.append(f'<section class="problem" id="{lang}-{p["slug"]}"><div class="eyebrow">{p["tag"][lang]}</div><h3>{p["title"][lang]}</h3><p>{p["text"][lang]}</p><div class="compare">')
        out.append(f'<figure><figcaption><strong>alg</strong><span>{lines(alg)} {ui["lines"]}</span></figcaption>{code_block(alg, "alg")}</figure>')
        out.append(f'<figure class="cpp"><figcaption><strong>C++</strong><span>{lines(cpp)} {ui["lines"]}</span></figcaption>{code_block(cpp, "cpp")}</figure></div>')
        out.append('<ul class="notes">' + "".join(f"<li>{n}</li>" for n in p["notes"][lang]) + "</ul></section>")
    out.append(f'<section id="{lang}-techniques"><div class="eyebrow">{ui["tech_eyebrow"]}</div><h2>{ui["tech_title"]}</h2><p class="prose">{ui["tech_text"]}</p><div class="table-wrap"><table><thead><tr><th>{ui["col_call"]}</th><th>{ui["col_does"]}</th><th>{ui["col_cost"]}</th></tr></thead><tbody>')
    for en_name, pt_name, does, cost in TECHNIQUES:
        out.append(f"<tr><td><code>{en_name if lang == 'en' else pt_name}</code></td><td>{does[lang]}</td><td class=\"num\">{cost}</td></tr>")
    out.append("</tbody></table></div></section>")
    out.append(f'<section id="{lang}-translation"><div class="eyebrow">{ui["translation_eyebrow"]}</div><h2>{ui["translation_title"]}</h2><div class="table-wrap"><table><thead><tr><th>{ui["col_cpp"]}</th><th>{ui["col_alg"]}</th><th>{ui["col_note"]}</th></tr></thead><tbody>')
    for cpp, en, pt, note in TRANSLATION:
        out.append(f"<tr><td>{cpp}</td><td>{en if lang == 'en' else pt}</td><td>{note[lang]}</td></tr>")
    out.append("</tbody></table></div></section>")
    out.append(f'<section id="{lang}-pitfalls"><div class="eyebrow">{ui["pitfalls_eyebrow"]}</div><h2>{ui["pitfalls_title"]}</h2><div class="table-wrap"><table><thead><tr><th>{ui["col_in_cpp"]}</th><th>{ui["col_in_alg"]}</th></tr></thead><tbody>')
    for item in PITFALLS:
        a, b = item[lang]
        out.append(f"<tr><td>{a}</td><td>{b}</td></tr>")
    out.append("</tbody></table></div></section>")
    out.append(f'<section id="{lang}-speed"><div class="eyebrow">{ui["perf_eyebrow"]}</div><h2>{ui["perf_title"]}</h2><p class="prose">{ui["perf_text"]}</p><div class="table-wrap"><table><thead><tr><th>{ui["col_problem"]}</th><th>alg -O</th><th>C++ -O2</th><th>{ui["col_ratio"]}</th></tr></thead><tbody>')
    for name, a, c, r in PERF:
        out.append(f'<tr><td>{name[lang]}</td><td class="num">{number(a, lang)} s</td><td class="num">{number(c, lang)} s</td><td class="num">{number(r, lang)}×</td></tr>')
    out.append(f'</tbody></table></div><div class="callout"><p>{ui["perf_callout"]}</p></div></section>')
    out.append(f'<footer class="prose"><p>{ui["footer"]}</p></footer></div>')
    return "\n".join(out)


def main():
    template = open(os.path.join(HERE, "guide_template.html")).read()
    body = build("en") + "\n" + build("pt")
    page = template.replace("<!--CONTENT-->", body)
    with open(os.path.join(HERE, "guia.html"), "w") as f:
        f.write(page)
    print("docs/guia.html gerado")


if __name__ == "__main__":
    main()
