#ifndef ALG_H
#define ALG_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- utilidades ---------- */

typedef struct
{
    char *text;
    size_t count;
    size_t capacity;
} TextBuffer;

void text_append(TextBuffer *out, const char *stmt);
void text_appendf(TextBuffer *out, const char *format_string, ...);
void *allocate_zeroed(size_t size);
char *duplicate_string(const char *stmt);
_Noreturn void fail_at(int line, int column_of, const char *format_string, ...);

extern const char *source_file_name;

/* vetor dinâmico genérico: VEC(int) l; PUSH(l, 5); */
#define VEC(ItemType)                                                                              \
    struct                                                                                         \
    {                                                                                              \
        ItemType *items;                                                                           \
        int count;                                                                                 \
        int capacity;                                                                              \
    }

#define PUSH(list, item)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if ((list).count == (list).capacity)                                                       \
        {                                                                                          \
            (list).capacity = (list).capacity ? 2 * (list).capacity : 4;                           \
            (list).items = realloc((list).items, (size_t)(list).capacity * sizeof *(list).items);  \
        }                                                                                          \
        (list).items[(list).count++] = (item);                                                     \
    } while (0)

/* ---------- tokens ---------- */

typedef enum
{
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_INT,
    TOKEN_REAL,
    TOKEN_STRING,
    TOKEN_NAME,

    /* palavras-chave (o mesmo enum serve para pt e en) */

    KEYWORD_SE,
    KEYWORD_SENAO,
    KEYWORD_ENQUANTO,
    KEYWORD_PARA,
    KEYWORD_ATE,
    KEYWORD_EM,
    KEYWORD_THR,
    KEYWORD_FUNCAO,
    KEYWORD_RET,
    KEYWORD_PARE,
    KEYWORD_CONTINUE,
    KEYWORD_E,
    KEYWORD_OU,
    KEYWORD_NAO,
    KEYWORD_VERDADEIRO,
    KEYWORD_FALSO,
    KEYWORD_DECL,
    KEYWORD_INC,
    KEYWORD_DEC,

    /* reservadas para os próximos marcos */

    KEYWORD_TODO,
    KEYWORD_EXISTE,
    KEYWORD_ALGUM,
    KEYWORD_UNIAO,
    KEYWORD_INTER,
    KEYWORD_MENOS,
    KEYWORD_CONJ,
    KEYWORD_TIPO,

    /* pontuação */

    PUNCT_LEFT_PAREN,
    PUNCT_RIGHT_PAREN,
    PUNCT_LEFT_BRACKET,
    PUNCT_RIGHT_BRACKET,
    PUNCT_LEFT_BRACE,
    PUNCT_RIGHT_BRACE,
    PUNCT_COMMA,
    PUNCT_COLON,
    PUNCT_SEMICOLON,
    PUNCT_DOT,
    PUNCT_RANGE,
    PUNCT_ASSIGN,
    PUNCT_EQUAL,
    PUNCT_NOT_EQUAL,
    PUNCT_LESS,
    PUNCT_LESS_EQUAL,
    PUNCT_GREATER,
    PUNCT_GREATER_EQUAL,
    PUNCT_PLUS,
    PUNCT_MINUS,
    PUNCT_STAR,
    PUNCT_SLASH,
    PUNCT_PERCENT,
    PUNCT_AMPERSAND,
    PUNCT_BAR,
    PUNCT_CARET,
    PUNCT_TILDE,
    PUNCT_SHIFT_LEFT,
    PUNCT_SHIFT_RIGHT,

    /* atribuição composta, igual ao C (só como comando: x += 1) */

    PUNCT_PLUS_ASSIGN,
    PUNCT_MINUS_ASSIGN,
    PUNCT_STAR_ASSIGN,
    PUNCT_SLASH_ASSIGN,
    PUNCT_PERCENT_ASSIGN,
    PUNCT_AMPERSAND_ASSIGN,
    PUNCT_BAR_ASSIGN,
    PUNCT_CARET_ASSIGN,
    PUNCT_SHIFT_LEFT_ASSIGN,
    PUNCT_SHIFT_RIGHT_ASSIGN
} TokenKind;

typedef struct
{
    TokenKind kind;
    int line;
    int column;
    char *text;        /* nome ou texto */
    int64_t int_value; /* valor inteiro */
    double real_value; /* valor real */
} Token;

typedef VEC(Token) TokenList;

typedef enum
{
    LANG_PT,
    LANG_EN
} Language;

TokenList tokenize(const char *source, Language *language);
const char *token_name(TokenKind kind);

/* ---------- tipos ---------- */

typedef enum
{
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_TUPLE,
    TYPE_HEAP,
    TYPE_QUEUE,
    TYPE_GRAPH,
    TYPE_MAP
} TypeKind;

typedef struct Type Type;

/* TY_UNK é uma célula mutável: é refinada no lugar quando o uso revela o tipo */
struct Type
{
    TypeKind kind;
    Type *element;       /* vetor, heap, fila: elemento; mapa: chave; grafo: peso */
    int dimension_count; /* TY_ARR: número de dimensões */

    Type **fields; /* TY_TUP: tipos dos campos */
    int field_count;
    Type *value;    /* TY_MAP: tipo do valor */
    int descending; /* TY_HEAP: heap(maior) */
    int directed;   /* TY_GRAPH: grafo(n, dirigido) */
    int weighted;   /* TY_GRAPH: alguma aresta tem peso */

    /* célula que não é de vetor (heap, fila, mapa, grafo): nunca é estreitada */
    int is_structure_cell;

    /* célula de elemento de vetor inteiro: escolha do tamanho na memória */
    Type *link;    /* células ligadas precisam ter o mesmo tamanho (union-find) */
    int is_int32;  /* anotado com ': int32' */
    int is_ranged; /* lo..hi já tem algum valor */
    int growths;   /* quantas vezes a faixa cresceu (para alargar e terminar) */
    int64_t low;   /* faixa dos valores guardados */
    int64_t high;
};

/* ---------- AST ---------- */

typedef struct Expression Expression;
typedef struct Statement Statement;
typedef struct Variable Variable;
typedef struct FunctionInstance FunctionInstance;
typedef VEC(Expression *) ExpressionList;
typedef VEC(Statement *) StatementList;
typedef VEC(Variable *) VariableList;

typedef enum
{
    EXPR_INT,
    EXPR_REAL,
    EXPR_STRING,
    EXPR_NAME,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_METHOD,
    EXPR_INDEX,
    EXPR_SLICE,
    EXPR_VECTOR,
    EXPR_GENERATOR,
    EXPR_TUPLE,
    EXPR_MAP
} ExpressionKind;

/* Campos first/second/third de cada tipo de expressão: tabela em docs/compilador.md. */
struct Expression
{
    ExpressionKind kind;
    int line;
    int column;
    Type *type;
    int64_t int_value;
    double real_value;
    char *text; /* literal, nome, método ou função */
    TokenKind operation;
    Expression *first;
    Expression *second;
    Expression *third;
    ExpressionList arguments;   /* chamada, método, vetor literal */
    int inclusive;              /* E_GEN: intervalo inclui o fim; E_SLICE em ler(): cria o vetor */
    Variable *variable;         /* nome resolvido / variável do gerador */
    FunctionInstance *instance; /* chamada resolvida */
};

typedef struct
{
    char *name;
    ExpressionList dimensions;
    char *type_name;
    int line;
    int column;
} Declaration;

typedef enum
{
    STMT_EXPR,
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOR_IN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_RETURN,
    STMT_DECL,
    STMT_FUNCTION
} StatementKind;

/* Campos first/second/third de cada tipo de comando: tabela em docs/compilador.md. */
struct Statement
{
    StatementKind kind;
    int line;
    int column;
    Expression *first;
    Expression *second;
    Expression *third;
    char *text;
    int inclusive;
    StatementList body;
    StatementList else_body;
    VEC(Declaration) declarations;
    VEC(char *) parameters; /* S_FUNC: parâmetros; S_FORIN: nomes de (v, w) */
    Variable *variable;
    Variable **pattern_variables; /* S_FORIN com (v, w): as variáveis */
};

StatementList parse(TokenList *tokens, Language language);

/* ---------- semântica ---------- */

struct Variable
{
    char *name;
    Type *type;
    int is_global;
    int is_parameter;
    int is_lazy;          /* #decl v[n][m]: vetor alocado no primeiro uso */
    int used_in_function; /* citada dentro de alguma função: precisa ser global no C */
    int growths;          /* análise de faixa (range.c) */
    int64_t low;
    int64_t high;
    int line;
    ExpressionList dimensions;
};

typedef struct Function
{
    char *name;
    Statement *definition;
    FunctionInstance *instances;
    int instance_count;
} Function;

/* uma versão da função para cada combinação de tipos dos argumentos */
struct FunctionInstance
{
    Function *function;
    Type **parameter_types;
    Type *return_type;
    char *c_name;
    StatementList body;   /* cópia do corpo, anotada com os tipos desta versão */
    int is_being_checked; /* sendo checada agora (recursão) */
    VEC(Variable *) locals;
    VariableList writes; /* variáveis que ela altera (incluindo pelas funções que chama) */
    Variable **parameters;
    FunctionInstance *next;
};

typedef struct
{
    StatementList statements;
    VEC(Variable *) globals;
    VEC(FunctionInstance *) instances;
    Language language;
    int no_checks; /* -O: sem checagens, com pragmas de otimização */
    int uses_inf;  /* o programa usa INF */
} Program;

void check_program(Program *parsed_program);
void analyze_ranges(Program *parsed_program);
extern int narrowing_ok;
Type *cell_root(Type *tokens);
void generate_c(Program *parsed_program, TextBuffer *out);

/* ---------- nomes nativos bilíngues ---------- */

typedef enum
{
    BUILTIN_NONE,
    BUILTIN_LER,
    BUILTIN_ESCREVER,
    BUILTIN_TAMANHO,
    BUILTIN_COPIA,
    BUILTIN_SOMA,
    BUILTIN_MIN,
    BUILTIN_MAX,
    BUILTIN_ADICIONAR,
    BUILTIN_REMOVER_ULTIMO,
    BUILTIN_INT,
    BUILTIN_REAL,
    BUILTIN_INT32,
    BUILTIN_ORDENAR,
    BUILTIN_HEAP,
    BUILTIN_FILA,
    BUILTIN_GRAFO,
    BUILTIN_MAPA,
    BUILTIN_MAIOR,
    BUILTIN_DIRIGIDO,
    BUILTIN_INF
} Builtin;

/* métodos das estruturas: o nome depende do tipo do objeto */
typedef enum
{
    METHOD_NONE,
    METHOD_ADICIONAR,
    METHOD_REMOVER_ULTIMO,
    METHOD_INSERIR,
    METHOD_REMOVER,
    METHOD_TOPO,
    METHOD_ENFILEIRAR,
    METHOD_DESENFILEIRAR,
    METHOD_FRENTE,
    METHOD_ARESTA,
    METHOD_VIZINHOS
} Method;

Method method_of(TypeKind kind, const char *name, Language language);

/* INF: grande o bastante para "infinito", com folga para INF + peso não estourar */
#define ALG_INF 1000000000000000000LL

Builtin builtin_of(const char *name, Language language);

#endif
