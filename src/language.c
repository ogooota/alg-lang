/* Palavras da linguagem em cada idioma: palavras-chave, nomes nativos e métodos.
 * Um idioma novo é mais uma coluna nestas tabelas (veja o TODO em docs/linguagem.md). */
#include "alg.h"

/* ---------- tabelas ---------- */

/* índice = Tok - K_SE; NULL = a palavra não existe nesse idioma */
static const char *KEYWORDS[2][KEYWORD_TIPO - KEYWORD_SE + 1] = {
    {
        "se",   "senao",    "enquanto", "para",  "ate",   "em",         NULL,    "fn",   "ret",
        "pare", "continue", "e",        "ou",    "nao",   "verdadeiro", "falso", NULL,   NULL,
        NULL,  "todo",     "existe",   "algum", "uniao", "inter",      "menos", "conj", "tipo",
    },
    {
        "if",    "else",     "while",  "for",  "to",    "in",    "thr",   "fn",  "ret",
        "break", "continue", "and",    "or",   "not",   "true",  "false", NULL,  NULL,
        NULL,   "all",      "exists", "some", "union", "inter", "minus", "set", "type",
    },
};

/** Palavra-chave com esse nome no idioma, ou TOKEN_NAME se não é palavra-chave. */
TokenKind keyword_of(const char *name, Language language)
{
    for (int index = 0; index <= KEYWORD_TIPO - KEYWORD_SE; index++)
    {
        const char *keyword = KEYWORDS[language][index];

        if (keyword && !strcmp(keyword, name))
        {
            return (TokenKind)(KEYWORD_SE + index);
        }
    }

    return TOKEN_NAME;
}

/** Como a palavra-chave se escreve no idioma do programa (no outro, se só existe lá). */
const char *keyword_spelling(TokenKind kind)
{
    const char *own = KEYWORDS[message_language][kind - KEYWORD_SE];
    return own ? own : KEYWORDS[!message_language][kind - KEYWORD_SE];
}

/* ---------- nomes nativos: [pt, en] ---------- */

static const char *BUILTIN_NAMES[][2] = {
    [BUILTIN_LER] = {"ler", "read"},
    [BUILTIN_ESCREVER] = {"escrever", "write"},
    [BUILTIN_TAMANHO] = {"tam", "size"},
    [BUILTIN_COPIA] = {"copia", "copy"},
    [BUILTIN_SOMA] = {"soma", "sum"},
    [BUILTIN_MIN] = {"min", "min"},
    [BUILTIN_MAX] = {"max", "max"},
    [BUILTIN_ADICIONAR] = {"adicionar", "add"},
    [BUILTIN_REMOVER_ULTIMO] = {"remover_ultimo", "pop"},
    [BUILTIN_INT] = {"int", "int"},
    [BUILTIN_REAL] = {"real", "real"},
    [BUILTIN_INT32] = {"int32", "int32"},
    [BUILTIN_ORDENAR] = {"ordenar", "sort"},
    [BUILTIN_HEAP] = {"heap", "heap"},
    [BUILTIN_FILA] = {"fila", "queue"},
    [BUILTIN_GRAFO] = {"grafo", "graph"},
    [BUILTIN_MAPA] = {"mapa", "map"},
    [BUILTIN_MAIOR] = {"maior", "greater"},
    [BUILTIN_DIRIGIDO] = {"dirigido", "directed"},
    [BUILTIN_INF] = {"INF", "INF"},
    [BUILTIN_PREFIXO] = {"prefixo", "prefix"},
    [BUILTIN_BUSCA] = {"busca", "search"},
    [BUILTIN_MDC] = {"mdc", "gcd"},
    [BUILTIN_MMC] = {"mmc", "lcm"},
    [BUILTIN_POTMOD] = {"potmod", "powmod"},
    [BUILTIN_CRIVO] = {"crivo", "sieve"},
    [BUILTIN_CONJUNTOS] = {"conjuntos", "dsu"},
    [BUILTIN_FENWICK] = {"fenwick", "fenwick"},
    [BUILTIN_SEGTREE] = {"segtree", "segtree"},
    [BUILTIN_RMQ] = {"rmq", "rmq"},
    [BUILTIN_TEXTO] = {"texto", "str"},
    [BUILTIN_LER_LINHA] = {"ler_linha", "read_line"},
    [BUILTIN_CONTA_PRIMOS] = {"conta_primos", "count_primes"},
    [BUILTIN_COMPRIMIR] = {"comprimir", "compress"},
};

/** Qual nome nativo (ler, soma, heap...) é esse nome no idioma dado. */
Builtin builtin_of(const char *name, Language language)
{
    int count = (int)(sizeof BUILTIN_NAMES / sizeof *BUILTIN_NAMES);

    for (int i = 1; i < count; i++)
    {
        if (!strcmp(BUILTIN_NAMES[i][language], name))
        {
            return (Builtin)i;
        }
    }

    return BUILTIN_NONE;
}

/* ---------- métodos: [tipo do objeto, pt, en] ---------- */

typedef struct
{
    TypeKind type_kind;
    Method method;
    const char *name[2];
} MethodName;

static const MethodName METHOD_NAMES[] = {
    {TYPE_ARRAY, METHOD_ADICIONAR, {"adicionar", "add"}},
    {TYPE_ARRAY, METHOD_REMOVER_ULTIMO, {"remover_ultimo", "pop"}},
    {TYPE_HEAP, METHOD_INSERIR, {"inserir", "push"}},
    {TYPE_HEAP, METHOD_REMOVER, {"remover", "pop"}},
    {TYPE_HEAP, METHOD_TOPO, {"topo", "top"}},
    {TYPE_QUEUE, METHOD_ENFILEIRAR, {"enfileirar", "push"}},
    {TYPE_QUEUE, METHOD_DESENFILEIRAR, {"desenfileirar", "pop"}},
    {TYPE_QUEUE, METHOD_FRENTE, {"frente", "front"}},
    {TYPE_GRAPH, METHOD_ARESTA, {"aresta", "edge"}},
    {TYPE_GRAPH, METHOD_VIZINHOS, {"vizinhos", "neighbors"}},
    {TYPE_DSU, METHOD_UNIR, {"unir", "merge"}},
    {TYPE_DSU, METHOD_ACHAR, {"achar", "find"}},
    {TYPE_DSU, METHOD_JUNTOS, {"juntos", "same"}},
    {TYPE_DSU, METHOD_TAM_GRUPO, {"tam", "size"}},
    {TYPE_DSU, METHOD_GRUPOS, {"grupos", "groups"}},
    {TYPE_FENWICK, METHOD_SOMAR, {"somar", "add"}},
    {TYPE_FENWICK, METHOD_SOMA, {"soma", "sum"}},
    {TYPE_SEGTREE, METHOD_CONSULTA, {"consulta", "query"}},
    {TYPE_SEGTREE, METHOD_MUDAR, {"mudar", "update"}},
    {TYPE_RMQ, METHOD_CONSULTA, {"consulta", "query"}},
    {TYPE_MAP, METHOD_REMOVER, {"remover", "remove"}},
    {TYPE_MAP, METHOD_ADICIONAR, {"adicionar", "add"}},
};

/** Qual método é esse nome para o tipo de objeto e o idioma dados. */
Method method_of(TypeKind k, const char *name, Language language)
{
    int count = (int)(sizeof METHOD_NAMES / sizeof *METHOD_NAMES);

    for (int i = 0; i < count; i++)
    {
        if (METHOD_NAMES[i].type_kind == k && !strcmp(METHOD_NAMES[i].name[language], name))
        {
            return METHOD_NAMES[i].method;
        }
    }

    return METHOD_NONE;
}
