#include "alg.h"

#include "rt_arr.inc"
#include "rt_core.inc"
#include "rt_fenwick.inc"
#include "rt_rmq.inc"
#include "rt_segtree.inc"
#include "rt_graph.inc"
#include "rt_heap.inc"
#include "rt_map.inc"
#include "rt_queue.inc"

static int temp_counter;

static void gen_expression(TextBuffer *out, Expression *expr);
static void gen_lvalue(TextBuffer *out, Expression *expr);
static void gen_stored_value(TextBuffer *out, Type *cell, Expression *value);
static void gen_statements(TextBuffer *out, StatementList *statements, int depth);
static void gen_method_call(TextBuffer *out, Expression *expr);

/* ---------- nomes e tipos ---------- */

static void indent(TextBuffer *out, int depth)
{
    for (int i = 0; i < depth; i++)
    {
        text_append(out, "    ");
    }
}

/* identificador do usuário → C: prefixo + bytes não-ASCII como _xx */
static void emit_identifier(TextBuffer *out, const char *prefix, const char *name)
{
    text_append(out, prefix);

    for (const unsigned char *cursor = (const unsigned char *)name; *cursor; cursor++)
    {
        int alnum = (*cursor >= 'a' && *cursor <= 'z') || (*cursor >= 'A' && *cursor <= 'Z') ||
                    (*cursor >= '0' && *cursor <= '9');

        if (alnum || *cursor == '_')
        {
            char text[2] = {(char)*cursor, 0};
            text_append(out, text);
        }
        else
        {
            text_appendf(out, "_%02x", *cursor);
        }
    }
}

/** Tipo ainda desconhecido na hora de gerar vira inteiro. */
static Type *resolve_unknown_type(Type *type)
{
    if (type->kind == TYPE_UNKNOWN)
    {
        type->kind = TYPE_INT;
    }

    return type;
}

/* ---------- tipo de elemento (vetor, heap, fila, mapa, grafo) ---------- */

typedef struct
{
    const char *suffix;     /* prefixo das funções: array_<suffix>_push */
    const char *pascal;     /* nome do tipo: Array<Pascal> */
    const char *c_type;     /* tipo guardado na memória */
    const char *value_type; /* tipo do valor na linguagem (contas, o que sai do vetor) */
    const char *reader;     /* leitura de um elemento */
    const char *writer;     /* escrita de um elemento */
    const char *less_than;  /* ordem: LESS_THAN ou tuple_<campos>_less */
    const char *loader;     /* guardado → valor: "" ou conversão */
    const char *signature;  /* tupla: tipos dos campos ("ir" = int, real) */
    int kind;               /* 0 inteiro, 1 real, 2 tupla */
} ElementKind;

enum
{
    ELEMENT_U8,
    ELEMENT_I8,
    ELEMENT_U16,
    ELEMENT_I16,
    ELEMENT_U32,
    ELEMENT_I32,
    ELEMENT_I32INF,
    ELEMENT_I64,
    ELEMENT_REAL,
    ELEMENT_KIND_COUNT
};

static const ElementKind ELEMENT_KINDS[ELEMENT_KIND_COUNT] = {
    [ELEMENT_U8] = {"u8", "U8", "uint8_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                    "(int64_t)", "", 0},
    [ELEMENT_I8] = {"i8", "I8", "int8_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                    "(int64_t)", "", 0},
    [ELEMENT_U16] = {"u16", "U16", "uint16_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I16] = {"i16", "I16", "int16_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_U32] = {"u32", "U32", "uint32_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I32] = {"i32", "I32", "int32_t", "int64_t", "read_int32", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I32INF] = {"i32_inf", "I32Inf", "int32_t", "int64_t", "read_int32", "write_int",
                        "LESS_THAN", "decode_int32_inf", "", 0},
    [ELEMENT_I64] = {"i64", "I64", "int64_t", "int64_t", "read_int", "write_int", "LESS_THAN", "",
                     "", 0},
    [ELEMENT_REAL] = {"real", "Real", "double", "double", "read_real", "write_real", "LESS_THAN",
                      "", "", 1},
};

static Program *program;

/* templates de vetor usados (só esses entram no C gerado) */
static int element_used[ELEMENT_KIND_COUNT];

/* tuplas usadas: cada formato vira uma struct e um template de vetor */
static VEC(ElementKind *) tuples;

/** Texto formatado com um argumento, em memória nova. */
static char *format_string(const char *format_text, const char *argument)
{
    char *text = allocate_zeroed(strlen(format_text) + strlen(argument) + 1);
    sprintf(text, format_text, argument);
    return text;
}

/** Descrição C de um formato de tupla (registra a struct na primeira vez). */
static const ElementKind *tuple_element_kind(Type *type)
{
    char signature[64] = "";
    char snake[512] = "tuple";
    char pascal[512] = "Tuple";

    for (int i = 0; i < type->field_count && i < 63; i++)
    {
        int is_real = resolve_unknown_type(type->fields[i])->kind == TYPE_REAL;

        signature[i] = is_real ? 'r' : 'i';
        strcat(snake, is_real ? "_real" : "_int");
        strcat(pascal, is_real ? "Real" : "Int");
    }

    for (int i = 0; i < tuples.count; i++)
    {
        if (!strcmp(tuples.items[i]->signature, signature))
        {
            return tuples.items[i];
        }
    }

    ElementKind *kind = allocate_zeroed(sizeof *kind);
    kind->signature = duplicate_string(signature);
    kind->suffix = duplicate_string(snake);
    kind->pascal = duplicate_string(pascal);
    kind->c_type = kind->pascal;
    kind->value_type = kind->c_type;
    kind->reader = format_string("%s_read", snake);
    kind->writer = format_string("%s_write", snake);
    kind->less_than = format_string("%s_less", snake);
    kind->loader = "";
    kind->kind = 2;
    PUSH(tuples, kind);
    return kind;
}

/* menor tipo que guarda a faixa de valores provada pela análise (range.c) */
static int scalar_element_kind(Type *cell)
{
    if (cell->kind == TYPE_REAL)
    {
        return ELEMENT_REAL;
    }

    if (cell->is_int32)
    {
        return program->uses_inf ? ELEMENT_I32INF : ELEMENT_I32;
    }

    if (cell->is_structure_cell || !narrowing_ok)
    {
        return ELEMENT_I64;
    }

    if (cell->low >= 0)
    {
        return cell->high <= UINT8_MAX    ? ELEMENT_U8
               : cell->high <= UINT16_MAX ? ELEMENT_U16
               : cell->high <= UINT32_MAX ? ELEMENT_U32
                                          : ELEMENT_I64;
    }

    if (cell->low >= INT8_MIN && cell->high <= INT8_MAX)
    {
        return ELEMENT_I8;
    }

    if (cell->low >= INT16_MIN && cell->high <= INT16_MAX)
    {
        return ELEMENT_I16;
    }

    if (cell->low >= INT32_MIN && cell->high <= INT32_MAX)
    {
        return ELEMENT_I32;
    }

    return ELEMENT_I64;
}

/** Descrição C do elemento guardado na célula (tamanho escolhido pela faixa). */
static const ElementKind *cell_element_kind(Type *cell)
{
    Type *root = cell_root(resolve_unknown_type(cell));

    if (root->kind == TYPE_TUPLE)
    {
        return tuple_element_kind(root);
    }

    int kind_index = scalar_element_kind(root);
    element_used[kind_index] = 1;
    return &ELEMENT_KINDS[kind_index];
}

/* sufixo do tipo de vetor: arr_u8, arr_i32, arr_i (64 bits), arr_r, arr_tii ... */
static const char *array_suffix(Type *array_type)
{
    return cell_element_kind(array_type->element)->suffix;
}

/** Nome do tipo C do vetor: ArrayU8, ArrayI64, ArrayTupleIntInt... */
static const char *array_pascal(Type *array_type)
{
    return cell_element_kind(array_type->element)->pascal;
}

/* ': int32' anotado: o valor guardado é checado (sem -O) */
static int is_checked_int32(Type *cell)
{
    return cell_root(cell)->is_int32;
}

/* ---------- estruturas: cada combinação de tipos instancia um template ---------- */

enum
{
    TEMPLATE_HEAP,
    TEMPLATE_QUEUE,
    TEMPLATE_MAP,
    TEMPLATE_GRAPH,
    TEMPLATE_FENWICK,
    TEMPLATE_SEGTREE,
    TEMPLATE_RMQ
};

typedef struct
{
    int kind;
    char prefix[96];            /* prefixo das funções: heap_i64_min_push */
    char type_name[96];         /* nome do tipo: HeapI64Min */
    const ElementKind *element; /* elemento / chave / peso */
    const ElementKind *value;   /* valor do mapa */
    int descending;             /* heap(maior) */
} TemplateUse;

static VEC(TemplateUse) templates;

/** Registra (uma vez) a instância de um template de estrutura e a devolve. */
static const TemplateUse *use_template(int kind, const ElementKind *element_kind,
                                       const ElementKind *value_kind, int descending)
{
    TemplateUse use = {0};
    const char *order = descending ? "max" : "min";
    const char *order_pascal = descending ? "Max" : "Min";

    static const char *const COMBINE_NAMES[] = {"min", "max", "sum"};
    static const char *const COMBINE_PASCAL[] = {"Min", "Max", "Sum"};

    if (kind == TEMPLATE_FENWICK)
    {
        snprintf(use.prefix, sizeof use.prefix, "fenwick_%s", element_kind->suffix);
        snprintf(use.type_name, sizeof use.type_name, "Fenwick%s", element_kind->pascal);
    }
    else if (kind == TEMPLATE_SEGTREE || kind == TEMPLATE_RMQ)
    {
        const char *base = kind == TEMPLATE_SEGTREE ? "segtree" : "rmq";
        const char *base_pascal = kind == TEMPLATE_SEGTREE ? "Segtree" : "Rmq";

        snprintf(use.prefix, sizeof use.prefix, "%s_%s_%s", base, element_kind->suffix,
                 COMBINE_NAMES[descending]);
        snprintf(use.type_name, sizeof use.type_name, "%s%s%s", base_pascal, element_kind->pascal,
                 COMBINE_PASCAL[descending]);
    }
    else if (kind == TEMPLATE_HEAP)
    {
        snprintf(use.prefix, sizeof use.prefix, "heap_%s_%s", element_kind->suffix, order);
        snprintf(use.type_name, sizeof use.type_name, "Heap%s%s", element_kind->pascal,
                 order_pascal);
    }
    else if (kind == TEMPLATE_QUEUE)
    {
        snprintf(use.prefix, sizeof use.prefix, "queue_%s", element_kind->suffix);
        snprintf(use.type_name, sizeof use.type_name, "Queue%s", element_kind->pascal);
    }
    else if (kind == TEMPLATE_MAP)
    {
        snprintf(use.prefix, sizeof use.prefix, "map_%s_%s", element_kind->suffix,
                 value_kind->suffix);
        snprintf(use.type_name, sizeof use.type_name, "Map%s%s", element_kind->pascal,
                 value_kind->pascal);
    }
    else
    {
        snprintf(use.prefix, sizeof use.prefix, "graph_%s", element_kind->suffix);
        snprintf(use.type_name, sizeof use.type_name, "Graph%s", element_kind->pascal);
    }

    for (int i = 0; i < templates.count; i++)
    {
        if (!strcmp(templates.items[i].prefix, use.prefix))
        {
            return &templates.items[i];
        }
    }

    use.kind = kind;
    use.element = element_kind;
    use.value = value_kind;
    use.descending = descending;
    PUSH(templates, use);
    return &templates.items[templates.count - 1];
}

/** Instância de template que implementa o tipo de estrutura (heap, fila, mapa, grafo). */
static const TemplateUse *structure_template(Type *type)
{
    switch (type->kind)
    {
    case TYPE_HEAP:
        return use_template(TEMPLATE_HEAP, cell_element_kind(type->element), NULL,
                            type->descending);

    case TYPE_QUEUE:
        return use_template(TEMPLATE_QUEUE, cell_element_kind(type->element), NULL, 0);

    case TYPE_MAP:
        return use_template(TEMPLATE_MAP, cell_element_kind(type->element),
                            cell_element_kind(type->value), 0);

    case TYPE_FENWICK:
        return use_template(TEMPLATE_FENWICK, cell_element_kind(type->element), NULL, 0);

    case TYPE_SEGTREE:
        return use_template(TEMPLATE_SEGTREE, cell_element_kind(type->element), NULL,
                            type->combine);

    case TYPE_RMQ:
        return use_template(TEMPLATE_RMQ, cell_element_kind(type->element), NULL, type->combine);

    default:
        return use_template(TEMPLATE_GRAPH, cell_element_kind(type->element), NULL, 0);
    }
}

/** Prefixo das funções da estrutura: heap_i64_min, map_i64_i64, graph_i64... */
static const char *template_name(Type *type)
{
    return structure_template(type)->prefix;
}

/** Nome do tipo C da estrutura: HeapI64Min, MapI64I64, GraphI64... */
static const char *template_type(Type *type)
{
    return structure_template(type)->type_name;
}

/** Diz se o tipo é heap, fila, mapa ou grafo (ponteiro no C). */
static int is_structure_type(Type *type)
{
    return type->kind == TYPE_HEAP || type->kind == TYPE_QUEUE || type->kind == TYPE_MAP ||
           type->kind == TYPE_GRAPH || type->kind == TYPE_DSU || type->kind == TYPE_FENWICK ||
           type->kind == TYPE_SEGTREE || type->kind == TYPE_RMQ;
}

/** Escreve o tipo C de um tipo da linguagem. */
static void emit_c_type(TextBuffer *out, Type *type)
{
    switch (resolve_unknown_type(type)->kind)
    {
    case TYPE_INT:
        text_append(out, "int64_t");
        break;

    case TYPE_REAL:
        text_append(out, "double");
        break;

    case TYPE_VOID:
        text_append(out, "void");
        break;

    case TYPE_ARRAY:
        text_appendf(out, "Array%s", array_pascal(type));
        break;

    case TYPE_TUPLE:
        text_append(out, tuple_element_kind(type)->c_type);
        break;

    case TYPE_DSU:
        text_append(out, "DisjointSets *");
        break;

    case TYPE_HEAP:
    case TYPE_QUEUE:
    case TYPE_MAP:
    case TYPE_GRAPH:
    case TYPE_FENWICK:
    case TYPE_SEGTREE:
    case TYPE_RMQ:
        text_appendf(out, "%s *", template_type(type));
        break;

    default:
        text_append(out, "int64_t");
        break;
    }
}

/** Escreve um texto como literal C, escapando o necessário. */
static void emit_c_string(TextBuffer *out, const char *text)
{
    text_append(out, "\"");

    for (const unsigned char *cursor = (const unsigned char *)text; *cursor; cursor++)
    {
        if (*cursor == '"' || *cursor == '\\')
        {
            text_appendf(out, "\\%c", *cursor);
        }
        else if (*cursor == '\n')
        {
            text_append(out, "\\n");
        }
        else if (*cursor == '\t')
        {
            text_append(out, "\\t");
        }
        else if (*cursor < 0x20)
        {
            text_appendf(out, "\\%03o", *cursor);
        }
        else
        {
            char character[2] = {(char)*cursor, 0};
            text_append(out, character);
        }
    }

    text_append(out, "\"");
}

/* ---------- variáveis ---------- */

typedef VariableList VariableSet;

/* vetores do #decl já garantidos no trecho que está sendo gerado */
static VariableSet ensured_lazy;

/** Diz se a variável está no conjunto. */
static int set_contains(VariableSet *set, Variable *variable)
{
    for (int i = 0; i < set->count; i++)
    {
        if (set->items[i] == variable)
        {
            return 1;
        }
    }

    return 0;
}

/* devolve 1 se v era novo */
static int set_add(VariableSet *set, Variable *variable)
{
    if (set_contains(set, variable))
    {
        return 0;
    }

    PUSH(*set, variable);
    return 1;
}

/** Escreve a criação de um vetor do #decl com os tamanhos dele. */
static void emit_lazy_allocation(TextBuffer *out, Variable *variable, int line)
{
    text_appendf(out, "array_%s_new(%d, %d", array_suffix(variable->type), line,
                 variable->dimensions.count);

    for (int i = 0; i < 4; i++)
    {
        text_append(out, ", ");

        if (i < variable->dimensions.count)
        {
            gen_expression(out, variable->dimensions.items[i]);
        }
        else
        {
            text_append(out, "0");
        }
    }

    text_append(out, ")");
}

/* #decl v[n]: o vetor é criado no primeiro acesso */
static void emit_lazy_ensure(TextBuffer *out, Variable *variable, int line)
{
    text_append(out, "(");
    emit_identifier(out, "v_", variable->name);
    text_append(out, ".data || (");
    emit_identifier(out, "v_", variable->name);
    text_append(out, " = ");
    emit_lazy_allocation(out, variable, line);
    text_append(out, ", 1))");
}

/** Escreve o acesso a uma variável (com a criação no primeiro uso, se for #decl). */
static void gen_variable(TextBuffer *out, Variable *variable, int line)
{
    if (variable->is_parameter && variable->type->kind == TYPE_ARRAY)
    {
        text_append(out, "(*");
        emit_identifier(out, "v_", variable->name);
        text_append(out, ")");
    }
    else if (variable->is_lazy && !set_contains(&ensured_lazy, variable))
    {
        text_append(out, "(*(");
        emit_lazy_ensure(out, variable, line);
        text_append(out, ", &");
        emit_identifier(out, "v_", variable->name);
        text_append(out, "))");
    }
    else
    {
        emit_identifier(out, "v_", variable->name);
    }
}

/* ponteiro para vetor: parâmetros de função e métodos que alteram o vetor */
static void gen_array_pointer(TextBuffer *out, Expression *expr)
{
    if (expr->kind == EXPR_NAME && expr->variable->is_parameter)
    {
        emit_identifier(out, "v_", expr->variable->name);
        return;
    }

    if (expr->kind == EXPR_NAME && expr->variable->is_lazy &&
        !set_contains(&ensured_lazy, expr->variable))
    {
        text_append(out, "(");
        emit_lazy_ensure(out, expr->variable, expr->line);
        text_append(out, ", &");
        emit_identifier(out, "v_", expr->variable->name);
        text_append(out, ")");
        return;
    }

    if (expr->kind == EXPR_NAME)
    {
        text_append(out, "&");
        emit_identifier(out, "v_", expr->variable->name);
        return;
    }

    text_appendf(out, "array_%s_tmp(&(Array%s){0}, ", array_suffix(expr->type),
                 array_pascal(expr->type));
    gen_expression(out, expr);
    text_append(out, ")");
}

/* ---------- expressões ---------- */

static const char *c_operator(TokenKind operation)
{
    switch (operation)
    {
    case KEYWORD_E:
        return "&&";

    case KEYWORD_OU:
        return "||";

    case PUNCT_EQUAL:
        return "==";

    case PUNCT_NOT_EQUAL:
        return "!=";

    case PUNCT_LESS:
        return "<";

    case PUNCT_LESS_EQUAL:
        return "<=";

    case PUNCT_GREATER:
        return ">";

    case PUNCT_GREATER_EQUAL:
        return ">=";

    case PUNCT_PLUS:
        return "+";

    case PUNCT_MINUS:
        return "-";

    case PUNCT_STAR:
        return "*";

    case PUNCT_AMPERSAND:
        return "&";

    case PUNCT_BAR:
        return "|";

    case PUNCT_CARET:
        return "^";

    case PUNCT_SHIFT_LEFT:
        return "<<";

    case PUNCT_SHIFT_RIGHT:
        return ">>";

    default:
        return "?";
    }
}

/** Escreve fn(x, y). */
static void gen_call2(TextBuffer *out, const char *function_suffix, Expression *item,
                      Expression *right)
{
    text_appendf(out, "%s(", function_suffix);
    gen_expression(out, item);
    text_append(out, ", ");
    gen_expression(out, right);
    text_append(out, ")");
}

/* comparação de tuplas: ordem lexicográfica (campo a campo) */
static void gen_tuple_comparison(TextBuffer *out, Expression *expr)
{
    const char *prefix = tuple_element_kind(expr->first->type)->suffix;
    char less_name[600];
    char equal_name[600];

    snprintf(less_name, sizeof less_name, "%s_less", prefix);
    snprintf(equal_name, sizeof equal_name, "%s_equal", prefix);

    switch (expr->operation)
    {
    case PUNCT_LESS:
        gen_call2(out, less_name, expr->first, expr->second);
        break;

    case PUNCT_GREATER:
        gen_call2(out, less_name, expr->second, expr->first);
        break;

    case PUNCT_LESS_EQUAL:
        text_append(out, "!");
        gen_call2(out, less_name, expr->second, expr->first);
        break;

    case PUNCT_GREATER_EQUAL:
        text_append(out, "!");
        gen_call2(out, less_name, expr->first, expr->second);
        break;

    case PUNCT_EQUAL:
        gen_call2(out, equal_name, expr->first, expr->second);
        break;

    default:
        text_append(out, "!");
        gen_call2(out, equal_name, expr->first, expr->second);
        break;
    }
}

/** Escreve uma operação binária (divisão e resto com a regra da linguagem). */
static void gen_binary(TextBuffer *out, Expression *expr)
{
    if (expr->operation == KEYWORD_EM)
    {
        text_appendf(out, "%s_has(", template_name(expr->second->type));
        gen_expression(out, expr->second);
        text_append(out, ", ");
        gen_expression(out, expr->first);
        text_append(out, ")");
        return;
    }

    if (expr->operation >= PUNCT_EQUAL && expr->operation <= PUNCT_GREATER_EQUAL &&
        expr->first->type->kind == TYPE_TUPLE)
    {
        gen_tuple_comparison(out, expr);
        return;
    }

    if (expr->operation == PUNCT_SLASH && expr->type->kind == TYPE_INT)
    {
        text_append(out, "floor_div(");
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_appendf(out, ", %d)", expr->line);
        return;
    }

    if (expr->operation == PUNCT_SLASH)
    {
        text_append(out, "((double)(");
        gen_expression(out, expr->first);
        text_append(out, ") / (");
        gen_expression(out, expr->second);
        text_append(out, "))");
        return;
    }

    if (expr->operation == PUNCT_PERCENT)
    {
        text_append(out, "euclid_mod(");
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_appendf(out, ", %d)", expr->line);
        return;
    }

    text_append(out, "(");
    gen_expression(out, expr->first);
    text_appendf(out, " %s ", c_operator(expr->operation));
    gen_expression(out, expr->second);
    text_append(out, ")");
}

/*
 * Laço de um gerador. Abre o laço e declara a variável;
 * quem chama escreve o corpo e fecha com "}".
 */
static void gen_generator_open(TextBuffer *out, Expression *generator, int temp_id)
{
    if (generator->second)
    {
        text_append(out, "for (int64_t ");
        emit_identifier(out, "v_", generator->text);
        text_append(out, " = ");
        gen_expression(out, generator->second);
        text_appendf(out, ", _h%d = ", temp_id);
        gen_expression(out, generator->third);
        text_append(out, "; ");
        emit_identifier(out, "v_", generator->text);
        text_appendf(out, " %s _h%d; ", generator->inclusive ? "<=" : "<", temp_id);
        emit_identifier(out, "v_", generator->text);
        text_append(out, "++) { ");
        return;
    }

    Type *iterable_type = generator->third->type;
    text_appendf(out, "Array%s _a%d = ", array_pascal(iterable_type), temp_id);
    gen_expression(out, generator->third);
    text_appendf(out, "; for (int64_t _k%d = 0; _k%d < _a%d.length; _k%d++) { ", temp_id, temp_id,
                 temp_id, temp_id);
    emit_c_type(out, generator->variable->type);
    text_append(out, " ");
    emit_identifier(out, "v_", generator->text);

    if (iterable_type->dimension_count == 1)
    {
        text_appendf(out, " = %s(_a%d.data[_k%d]); ",
                     cell_element_kind(iterable_type->element)->loader, temp_id, temp_id);
    }
    else
    {
        text_appendf(out, " = array_%s_row(_a%d, _k%d, %d); ", array_suffix(iterable_type), temp_id,
                     temp_id, generator->line);
    }
}

/** Escreve soma/min/max de vetor, de vários valores ou de um gerador. */
static void gen_reduce(TextBuffer *out, Expression *expr, Builtin builtin)
{
    const char *name = builtin == BUILTIN_SOMA ? "sum" : builtin == BUILTIN_MIN ? "min" : "max";

    if (expr->arguments.count > 1)
    {
        /* min(a, b, c) → rt_min(rt_min(a, b), c) */
        const char *type_letter = expr->type->kind == TYPE_REAL ? "real" : "int";

        for (int i = 1; i < expr->arguments.count; i++)
        {
            text_appendf(out, "%s_%s(", name, type_letter);
        }

        gen_expression(out, expr->arguments.items[0]);

        for (int i = 1; i < expr->arguments.count; i++)
        {
            text_append(out, ", ");
            gen_expression(out, expr->arguments.items[i]);
            text_append(out, ")");
        }

        return;
    }

    Expression *argument = expr->arguments.items[0];

    if (argument->kind != EXPR_GENERATOR)
    {
        text_appendf(out, "array_%s_%s(", array_suffix(argument->type), name);
        gen_expression(out, argument);
        text_appendf(out, ", %d)", expr->line);
        return;
    }

    int temp_id = temp_counter++;
    text_append(out, "({ ");
    emit_c_type(out, expr->type);

    if (builtin == BUILTIN_SOMA)
    {
        text_appendf(out, " _s%d = 0; ", temp_id);
        gen_generator_open(out, argument, temp_id);
        text_appendf(out, "_s%d += ", temp_id);
        gen_expression(out, argument->first);
        text_appendf(out, "; } _s%d; })", temp_id);
        return;
    }

    text_appendf(out, " _s%d = 0; int _f%d = 0; ", temp_id, temp_id);
    gen_generator_open(out, argument, temp_id);
    emit_c_type(out, expr->type);
    text_appendf(out, " _x%d = ", temp_id);
    gen_expression(out, argument->first);
    text_appendf(out, "; if (!_f%d || _x%d %s _s%d) { _s%d = _x%d; _f%d = 1; } } ", temp_id,
                 temp_id, builtin == BUILTIN_MIN ? "<" : ">", temp_id, temp_id, temp_id, temp_id);
    text_appendf(out, "if (!_f%d) runtime_error(%d, \"%s de faixa vazia\"); _s%d; })", temp_id,
                 expr->line, name, temp_id);
}

/** Escreve uma chamada de função nativa ou do usuário. */
/** Como ler um elemento guardado do vetor como valor da linguagem (texto "loader(x)"). */
static void gen_loaded(TextBuffer *out, Type *array, const char *element)
{
    text_appendf(out, "%s(%s)", cell_element_kind(array->element)->loader, element);
}

/** prefixo(v): P[i] = soma de v[0..i); em matriz, o prefixo 2D. */
static void gen_prefix(TextBuffer *out, Expression *expr)
{
    Type *source = expr->arguments.items[0]->type;
    const char *pascal = array_pascal(source);
    const char *result_suffix = array_suffix(expr->type);
    const char *result_pascal = array_pascal(expr->type);
    int temp_id = temp_counter++;
    char element[96];

    text_appendf(out, "({ Array%s _s%d = ", pascal, temp_id);
    gen_expression(out, expr->arguments.items[0]);

    if (source->dimension_count == 1)
    {
        text_appendf(out, "; Array%s _p%d = array_%s_new(%d, 1, _s%d.length + 1, 0, 0, 0); ",
                     result_pascal, temp_id, result_suffix, expr->line, temp_id);
        text_appendf(out, "for (int64_t _k%d = 0; _k%d < _s%d.length; _k%d++) { ", temp_id, temp_id,
                     temp_id, temp_id);
        text_appendf(out, "_p%d.data[_k%d + 1] = _p%d.data[_k%d] + ", temp_id, temp_id, temp_id,
                     temp_id);
        snprintf(element, sizeof element, "_s%d.data[_k%d]", temp_id, temp_id);
        gen_loaded(out, source, element);
        text_appendf(out, "; } _p%d; })", temp_id);
        return;
    }

    /* 2D: P[i+1][j+1] = S[i][j] + P[i][j+1] + P[i+1][j] - P[i][j], em índices planos */
    text_appendf(out, "; int64_t _n%d = _s%d.length; int64_t _m%d = _s%d.inner_sizes[0]; ", temp_id,
                 temp_id, temp_id, temp_id);
    text_appendf(out, "Array%s _p%d = array_%s_new(%d, 2, _n%d + 1, _m%d + 1, 0, 0); ",
                 result_pascal, temp_id, result_suffix, expr->line, temp_id, temp_id);
    text_appendf(out, "for (int64_t _i%d = 0; _i%d < _n%d; _i%d++) { ", temp_id, temp_id, temp_id,
                 temp_id);
    text_appendf(out, "for (int64_t _j%d = 0; _j%d < _m%d; _j%d++) { ", temp_id, temp_id, temp_id,
                 temp_id);
    text_appendf(out, "int64_t _w%d = _m%d + 1; ", temp_id, temp_id);
    text_appendf(out, "_p%d.data[(_i%d + 1) * _w%d + _j%d + 1] = ", temp_id, temp_id, temp_id,
                 temp_id);
    snprintf(element, sizeof element, "_s%d.data[_i%d * _m%d + _j%d]", temp_id, temp_id, temp_id,
             temp_id);
    gen_loaded(out, source, element);
    text_appendf(out, " + _p%d.data[_i%d * _w%d + _j%d + 1] + _p%d.data[(_i%d + 1) * _w%d + _j%d]",
                 temp_id, temp_id, temp_id, temp_id, temp_id, temp_id, temp_id, temp_id);
    text_appendf(out, " - _p%d.data[_i%d * _w%d + _j%d]; } } _p%d; })", temp_id, temp_id, temp_id,
                 temp_id, temp_id);
}

/** busca(v, x): primeira posição com v[i] >= x; com 'maior', com v[i] > x. */
static void gen_search(TextBuffer *out, Expression *expr)
{
    Type *source = expr->arguments.items[0]->type;
    const ElementKind *kind = cell_element_kind(source->element);
    int upper = expr->arguments.count == 3;
    int temp_id = temp_counter++;
    char element[64];

    text_appendf(out, "({ Array%s _a%d = ", array_pascal(source), temp_id);
    gen_expression(out, expr->arguments.items[0]);
    text_append(out, "; ");
    emit_c_type(out, expr->arguments.items[1]->type);
    text_appendf(out, " _x%d = ", temp_id);
    gen_expression(out, expr->arguments.items[1]);
    text_appendf(out, "; int64_t _lo%d = 0; int64_t _hi%d = _a%d.length; ", temp_id, temp_id,
                 temp_id);
    text_appendf(out, "while (_lo%d < _hi%d) { int64_t _m%d = _lo%d + (_hi%d - _lo%d) / 2; ",
                 temp_id, temp_id, temp_id, temp_id, temp_id, temp_id);
    snprintf(element, sizeof element, "_a%d.data[_m%d]", temp_id, temp_id);

    /* invariante: antes de _lo vem tudo que é < x (ou <= x com 'maior'); de _hi em diante, não */
    if (upper)
    {
        text_appendf(out, "if (!%s(_x%d, ", kind->less_than, temp_id);
        gen_loaded(out, source, element);
        text_append(out, "))");
    }
    else
    {
        text_appendf(out, "if (%s(", kind->less_than);
        gen_loaded(out, source, element);
        text_appendf(out, ", _x%d))", temp_id);
    }

    text_appendf(out, " _lo%d = _m%d + 1; else _hi%d = _m%d; } _lo%d; })", temp_id, temp_id,
                 temp_id, temp_id, temp_id);
}

/** segtree(v, op) e rmq(v, op): cria, copia os valores de v para as folhas e monta. */
static void gen_range_structure(TextBuffer *out, Expression *expr)
{
    Type *source = expr->arguments.items[0]->type;
    const char *prefix = template_name(expr->type);
    int is_rmq = expr->type->kind == TYPE_RMQ;
    int temp_id = temp_counter++;
    char element[64];

    text_appendf(out, "({ Array%s _a%d = ", array_pascal(source), temp_id);
    gen_expression(out, expr->arguments.items[0]);
    text_appendf(out, "; %s *_t%d = %s_new(_a%d.length); ", template_type(expr->type), temp_id,
                 prefix, temp_id);
    text_appendf(out, "for (int64_t _k%d = 0; _k%d < _a%d.length; _k%d++) { ", temp_id, temp_id,
                 temp_id, temp_id);

    if (is_rmq)
    {
        text_appendf(out, "_t%d->level[0][_k%d] = ", temp_id, temp_id);
    }
    else
    {
        text_appendf(out, "_t%d->tree[_a%d.length + _k%d] = ", temp_id, temp_id, temp_id);
    }

    snprintf(element, sizeof element, "_a%d.data[_k%d]", temp_id, temp_id);
    gen_loaded(out, source, element);
    text_appendf(out, "; } %s_build(_t%d); })", prefix, temp_id);
}

static void gen_call(TextBuffer *out, Expression *expr)
{
    Builtin builtin = builtin_of(expr->text, LANG_PT) ? builtin_of(expr->text, LANG_PT)
                                                      : builtin_of(expr->text, LANG_EN);

    if (!expr->instance)
    {
        switch (builtin)
        {
        case BUILTIN_TAMANHO:
            text_append(out, "(");
            gen_expression(out, expr->arguments.items[0]);
            Type *measured = expr->arguments.items[0]->type;
            text_append(out, measured->kind == TYPE_GRAPH  ? ")->vertex_count"
                             : is_structure_type(measured) ? ")->count"
                                                           : ").length");
            return;

        case BUILTIN_ORDENAR:
            text_appendf(out, "array_%s_sort(", array_suffix(expr->arguments.items[0]->type));
            gen_expression(out, expr->arguments.items[0]);
            text_appendf(out, ", %d, %d)", expr->arguments.count == 2, expr->line);
            return;

        case BUILTIN_PREFIXO:
            gen_prefix(out, expr);
            return;

        case BUILTIN_BUSCA:
            gen_search(out, expr);
            return;

        case BUILTIN_MDC:
        case BUILTIN_MMC:
            text_append(out, builtin == BUILTIN_MDC ? "gcd_int(" : "lcm_int(");
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, ", ");
            gen_expression(out, expr->arguments.items[1]);
            text_append(out, ")");
            return;

        case BUILTIN_POTMOD:
            text_append(out, "powmod_int(");

            for (int i = 0; i < 3; i++)
            {
                gen_expression(out, expr->arguments.items[i]);
                text_append(out, ", ");
            }

            text_appendf(out, "%d)", expr->line);
            return;

        case BUILTIN_CRIVO:
        {
            int temp_id = temp_counter++;
            text_appendf(out, "({ int64_t _c%d; int64_t *_p%d = sieve_primes(", temp_id, temp_id);
            gen_expression(out, expr->arguments.items[0]);
            text_appendf(out, ", &_c%d, %d); array_%s_lit(_c%d, _p%d); })", temp_id, expr->line,
                         array_suffix(expr->type), temp_id, temp_id);
            return;
        }

        case BUILTIN_CONJUNTOS:
        case BUILTIN_FENWICK:
            text_appendf(out, "%s_new(",
                         builtin == BUILTIN_CONJUNTOS ? "dsu" : template_name(expr->type));
            gen_expression(out, expr->arguments.items[0]);
            text_appendf(out, ", %d)", expr->line);
            return;

        case BUILTIN_SEGTREE:
        case BUILTIN_RMQ:
            gen_range_structure(out, expr);
            return;

        case BUILTIN_HEAP:
        case BUILTIN_FILA:
            text_appendf(out, "%s_new()", template_name(expr->type));
            return;

        case BUILTIN_GRAFO:
            text_appendf(out, "%s_new(", template_name(expr->type));
            gen_expression(out, expr->arguments.items[0]);
            text_appendf(out, ", %d, %d, %d)", expr->type->directed, expr->type->weighted,
                         expr->line);
            return;

        case BUILTIN_COPIA:
            text_appendf(out, "array_%s_copy(", array_suffix(expr->type));
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, ")");
            return;

        case BUILTIN_SOMA:
        case BUILTIN_MIN:
        case BUILTIN_MAX:
            gen_reduce(out, expr, builtin);
            return;

        case BUILTIN_INT:
        case BUILTIN_INT32:
            text_append(out, expr->arguments.items[0]->type->kind == TYPE_REAL ? "((int64_t)floor("
                                                                               : "((int64_t)(");
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, "))");
            return;

        case BUILTIN_REAL:
            text_append(out, "((double)(");
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, "))");
            return;

        default:
            break;
        }
    }

    emit_identifier(out, "f_", expr->instance->c_name);
    text_append(out, "(");

    for (int i = 0; i < expr->arguments.count; i++)
    {
        if (i)
        {
            text_append(out, ", ");
        }

        if (expr->instance->parameter_types[i]->kind == TYPE_ARRAY)
        {
            gen_array_pointer(out, expr->arguments.items[i]);
        }
        else
        {
            gen_expression(out, expr->arguments.items[i]);
        }
    }

    text_append(out, ")");
}

/* posição de vetor como lugar onde se escreve (sem a conversão de leitura) */
static void gen_lvalue(TextBuffer *out, Expression *expr)
{
    if (expr->kind == EXPR_INDEX && expr->first->type->kind == TYPE_MAP)
    {
        text_appendf(out, "(*%s_ref(", template_name(expr->first->type));
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_append(out, "))");
        return;
    }

    if (expr->kind != EXPR_INDEX || expr->first->type->dimension_count != 1)
    {
        gen_expression(out, expr);
        return;
    }

    text_appendf(out, "(*array_%s_at(", array_suffix(expr->first->type));
    gen_expression(out, expr->first);
    text_append(out, ", ");
    gen_expression(out, expr->second);
    text_appendf(out, ", %d))", expr->line);
}

/* valor que vai ser guardado numa célula: com ': int32', confere se cabe */
static void gen_stored_value(TextBuffer *out, Type *cell, Expression *value)
{
    if (!is_checked_int32(cell))
    {
        gen_expression(out, value);
        return;
    }

    text_append(out, program->uses_inf ? "encode_int32_inf(" : "fit_int32(");
    gen_expression(out, value);
    text_appendf(out, ", %d)", value->line);
}

/** Escreve um método de vetor ou de estrutura. */
static void gen_method_call(TextBuffer *out, Expression *expr)
{
    Type *type = expr->first->type;
    Method method = method_of(type->kind, expr->text, program->language);

    if (type->kind == TYPE_ARRAY)
    {
        int is_push = method == METHOD_ADICIONAR;
        text_appendf(out, "array_%s_%s(", array_suffix(type), is_push ? "push" : "pop");
        gen_array_pointer(out, expr->first);

        if (is_push)
        {
            text_append(out, ", ");
            gen_stored_value(out, type->element, expr->arguments.items[0]);
        }

        text_appendf(out, ", %d)", expr->line);
        return;
    }

    if (method == METHOD_GRUPOS)
    {
        text_append(out, "(");
        gen_expression(out, expr->first);
        text_append(out, ")->groups");
        return;
    }

    const char *c_name = type->kind == TYPE_DSU ? "dsu" : template_name(type);
    const char *function_suffix = "?";

    switch (method)
    {
    case METHOD_UNIR:
        function_suffix = "merge";
        break;

    case METHOD_ACHAR:
        function_suffix = "find";
        break;

    case METHOD_JUNTOS:
        function_suffix = "same";
        break;

    case METHOD_TAM_GRUPO:
        function_suffix = "group_size";
        break;

    case METHOD_SOMAR:
        function_suffix = "add";
        break;

    case METHOD_SOMA:
        function_suffix = "sum";
        break;

    case METHOD_CONSULTA:
        function_suffix = "query";
        break;

    case METHOD_MUDAR:
        function_suffix = "set";
        break;

    case METHOD_INSERIR:
    case METHOD_ENFILEIRAR:
        function_suffix = "push";
        break;

    case METHOD_REMOVER:
        function_suffix = type->kind == TYPE_MAP ? "del" : "pop";
        break;

    case METHOD_DESENFILEIRAR:
        function_suffix = "pop";
        break;

    case METHOD_TOPO:
        function_suffix = "top";
        break;

    case METHOD_FRENTE:
        function_suffix = "front";
        break;

    case METHOD_ARESTA:
        function_suffix = "edge";
        break;

    default:
        break;
    }

    text_appendf(out, "%s_%s(", c_name, function_suffix);
    gen_expression(out, expr->first);

    for (int i = 0; i < expr->arguments.count; i++)
    {
        text_append(out, ", ");
        gen_expression(out, expr->arguments.items[i]);
    }

    if (method == METHOD_ARESTA && expr->arguments.count == 2)
    {
        text_append(out, ", 0");
    }

    /* push/del não precisam da linha; o resto pode dar erro (estrutura vazia, índice) */
    if (method != METHOD_INSERIR && method != METHOD_ENFILEIRAR &&
        !(method == METHOD_REMOVER && type->kind == TYPE_MAP))
    {
        text_appendf(out, ", %d", expr->line);
    }

    text_append(out, ")");
}

/** Escreve um número real como literal C. */
static void gen_real(TextBuffer *out, double number)
{
    char text[40];
    snprintf(text, sizeof text, "%.17g", number);

    if (!strpbrk(text, ".en"))
    {
        strcat(text, ".0");
    }

    text_append(out, text);
}

/** Escreve uma expressão em C. */
static void gen_expression(TextBuffer *out, Expression *expr)
{
    switch (expr->kind)
    {
    case EXPR_INT:
        text_appendf(out, "INT64_C(%lld)", (long long)expr->int_value);
        return;

    case EXPR_REAL:
        gen_real(out, expr->real_value);
        return;

    case EXPR_STRING:
        emit_c_string(out, expr->text);
        return;

    case EXPR_NAME:
        gen_variable(out, expr->variable, expr->line);
        return;

    case EXPR_BINARY:
        gen_binary(out, expr);
        return;

    case EXPR_UNARY:
        text_append(out, expr->operation == PUNCT_MINUS   ? "(-("
                         : expr->operation == PUNCT_TILDE ? "(~("
                                                          : "(!(");
        gen_expression(out, expr->first);
        text_append(out, "))");
        return;

    case EXPR_INDEX:
        if (expr->first->type->kind == TYPE_MAP)
        {
            text_appendf(out, "%s_get(", template_name(expr->first->type));
            gen_expression(out, expr->first);
            text_append(out, ", ");
            gen_expression(out, expr->second);
            text_append(out, ")");
        }
        else if (expr->first->type->dimension_count == 1)
        {
            /* elemento estreito volta a ser int64 (INF em int32 é decodificado) */
            text_appendf(out, "%s(", cell_element_kind(expr->first->type->element)->loader);
            gen_lvalue(out, expr);
            text_append(out, ")");
        }
        else
        {
            text_appendf(out, "array_%s_row(", array_suffix(expr->first->type));
            gen_expression(out, expr->first);
            text_append(out, ", ");
            gen_expression(out, expr->second);
            text_appendf(out, ", %d)", expr->line);
        }

        return;

    case EXPR_SLICE:
        text_appendf(out, "array_%s_slice(", array_suffix(expr->first->type));
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_append(out, ", ");
        gen_expression(out, expr->third);
        text_appendf(out, ", %d)", expr->line);
        return;

    case EXPR_VECTOR:
        if (!expr->arguments.count)
        {
            text_appendf(out, "array_%s_lit(0, NULL)", array_suffix(expr->type));
            return;
        }

        text_appendf(out, "array_%s_lit(%d, (%s[]){", array_suffix(expr->type),
                     expr->arguments.count, cell_element_kind(expr->type->element)->c_type);

        for (int i = 0; i < expr->arguments.count; i++)
        {
            text_append(out, i ? ", " : "");
            gen_stored_value(out, expr->type->element, expr->arguments.items[i]);
        }

        text_append(out, "})");
        return;

    case EXPR_METHOD:
        gen_method_call(out, expr);
        return;

    case EXPR_CALL:
        gen_call(out, expr);
        return;

    case EXPR_TUPLE:
        text_appendf(out, "((%s){", tuple_element_kind(expr->type)->c_type);

        for (int i = 0; i < expr->arguments.count; i++)
        {
            text_append(out, i ? ", " : "");
            gen_expression(out, expr->arguments.items[i]);
        }

        text_append(out, "})");
        return;

    case EXPR_MAP:
        text_appendf(out, "%s_new()", template_name(expr->type));
        return;

    case EXPR_GENERATOR:
        return;
    }
}

/* ---------- ler / escrever ---------- */

static void gen_read(TextBuffer *out, Expression *argument, int depth)
{
    indent(out, depth);

    if (argument->kind == EXPR_NAME && argument->variable->type->kind == TYPE_ARRAY)
    {
        text_appendf(out, "array_%s_read(", array_suffix(argument->variable->type));
        gen_variable(out, argument->variable, argument->line);
        text_appendf(out, ", %d);\n", argument->line);
        return;
    }

    if (argument->kind != EXPR_SLICE)
    {
        gen_lvalue(out, argument);
        text_appendf(out, " = %s(%d);\n", cell_element_kind(argument->type)->reader,
                     argument->line);
        return;
    }

    /* fatias: a[lo0..hi0][lo1..hi1] → sl[0] é a 1ª dimensão */
    Expression *slices[4];
    int dimension_count = 0;
    Expression *base = argument;

    while (base->kind == EXPR_SLICE)
    {
        base = base->first;
        dimension_count++;
    }

    base = argument;

    for (int i = dimension_count - 1; i >= 0; i--)
    {
        slices[i] = base;
        base = base->first;
    }

    Type *target_type = base->type;
    const char *suffix = array_suffix(target_type);
    const char *pascal = array_pascal(target_type);

    if (argument->inclusive)
    {
        emit_identifier(out, "v_", base->variable->name);
        text_appendf(out, " = array_%s_new(%d, %d", suffix, argument->line, dimension_count);

        for (int i = 0; i < 4; i++)
        {
            text_append(out, ", ");

            if (i < dimension_count)
            {
                gen_expression(out, slices[i]->third);
            }
            else
            {
                text_append(out, "0");
            }
        }

        text_append(out, ");\n");
        indent(out, depth);
    }

    int temp_id = temp_counter++;
    text_appendf(out, "{\n");
    indent(out, depth + 1);
    text_appendf(out, "Array%s _b%d_0 = ", pascal, temp_id);
    gen_expression(out, base);
    text_append(out, ";\n");

    for (int i = 0; i < dimension_count; i++)
    {
        indent(out, depth + 1 + i);
        text_appendf(out, "for (int64_t _i%d_%d = ", temp_id, i);
        gen_expression(out, slices[i]->second);
        text_appendf(out, ", _h%d_%d = ", temp_id, i);
        gen_expression(out, slices[i]->third);
        text_appendf(out, "; _i%d_%d < _h%d_%d; _i%d_%d++)\n", temp_id, i, temp_id, i, temp_id, i);
        indent(out, depth + 1 + i);
        text_append(out, "{\n");
        indent(out, depth + 2 + i);

        if (i < dimension_count - 1)
        {
            text_appendf(out, "Array%s _b%d_%d = array_%s_row(_b%d_%d, _i%d_%d, %d);\n", pascal,
                         temp_id, i + 1, suffix, temp_id, i, temp_id, i, argument->line);
        }
        else
        {
            text_appendf(out, "*array_%s_at(_b%d_%d, _i%d_%d, %d) = %s(%d);\n", suffix, temp_id, i,
                         temp_id, i, argument->line,
                         cell_element_kind(target_type->element)->reader, argument->line);
        }
    }

    for (int i = dimension_count - 1; i >= 0; i--)
    {
        indent(out, depth + 1 + i);
        text_append(out, "}\n");
    }

    indent(out, depth);
    text_append(out, "}\n");
}

/** Escreve escrever(a, b, ...): separa por espaço e termina com quebra de linha. */
static void gen_write(TextBuffer *out, Expression *expr, int depth)
{
    for (int i = 0; i < expr->arguments.count; i++)
    {
        Expression *argument = expr->arguments.items[i];
        indent(out, depth);

        if (i)
        {
            text_append(out, "write_space();\n");
            indent(out, depth);
        }

        if (argument->type->kind == TYPE_STRING)
        {
            text_append(out, "write_text(");
        }
        else if (argument->type->kind == TYPE_ARRAY)
        {
            text_appendf(out, "array_%s_write(", array_suffix(argument->type));
        }
        else if (argument->type->kind == TYPE_TUPLE)
        {
            text_appendf(out, "%s(", tuple_element_kind(argument->type)->writer);
        }
        else
        {
            text_append(out, resolve_unknown_type(argument->type)->kind == TYPE_REAL
                                 ? "write_real("
                                 : "write_int(");
        }

        gen_expression(out, argument);
        text_append(out, ");\n");
    }

    indent(out, depth);
    text_append(out, "write_newline();\n");
}

/* ---------- comandos ---------- */

static void gen_block(TextBuffer *out, StatementList *statements, int depth)
{
    indent(out, depth);
    text_append(out, "{\n");
    gen_statements(out, statements, depth + 1);
    indent(out, depth);
    text_append(out, "}\n");
}

/* ---------- #decl: a checagem do primeiro uso sai de dentro dos laços ---------- */

/*
 * Um vetor do #decl é criado no primeiro uso. Checar isso a cada acesso dentro
 * de um laço dobra o tempo do laço, então a checagem é feita uma vez antes dele,
 * desde que as variáveis do tamanho (o n de v[n]) não mudem dentro do laço.
 */

static void walk_statements(StatementList *statements, VariableSet *lazy, VariableSet *writes);

/* lazy: vetores do #decl usados; writes: variáveis alteradas (inclusive por funções chamadas) */
static void walk_expression(Expression *expr, VariableSet *lazy_arrays, VariableSet *writes)
{
    if (!expr)
    {
        return;
    }

    if (expr->kind == EXPR_NAME && expr->variable && expr->variable->is_lazy && lazy_arrays)
    {
        set_add(lazy_arrays, expr->variable);
    }

    if (expr->kind == EXPR_CALL && expr->instance && writes)
    {
        for (int i = 0; i < expr->instance->writes.count; i++)
        {
            set_add(writes, expr->instance->writes.items[i]);
        }
    }

    if (expr->kind == EXPR_METHOD && expr->first->kind == EXPR_NAME && writes)
    {
        set_add(writes, expr->first->variable);
    }

    walk_expression(expr->first, lazy_arrays, writes);
    walk_expression(expr->second, lazy_arrays, writes);
    walk_expression(expr->third, lazy_arrays, writes);

    for (int i = 0; i < expr->arguments.count; i++)
    {
        walk_expression(expr->arguments.items[i], lazy_arrays, writes);
    }
}

/** Percorre um comando registrando as escritas. */
static void walk_statement(Statement *stmt, VariableSet *lazy_arrays, VariableSet *writes)
{
    if (stmt->kind == STMT_FUNCTION || stmt->kind == STMT_DECL)
    {
        return;
    }

    if (writes && stmt->variable)
    {
        set_add(writes, stmt->variable);
    }

    if (writes && stmt->kind == STMT_ASSIGN && stmt->first->kind == EXPR_NAME)
    {
        set_add(writes, stmt->first->variable);
    }

    /* ler(x) e ler(v[0..n]) também alteram a variável */
    if (writes && stmt->kind == STMT_EXPR && stmt->first->kind == EXPR_CALL &&
        !stmt->first->instance)
    {
        for (int i = 0; i < stmt->first->arguments.count; i++)
        {
            Expression *argument = stmt->first->arguments.items[i];

            while (argument->kind == EXPR_SLICE || argument->kind == EXPR_INDEX)
            {
                argument = argument->first;
            }

            if (argument->kind == EXPR_NAME && argument->variable)
            {
                set_add(writes, argument->variable);
            }
        }
    }

    walk_expression(stmt->first, lazy_arrays, writes);
    walk_expression(stmt->second, lazy_arrays, writes);
    walk_expression(stmt->third, lazy_arrays, writes);
    walk_statements(&stmt->body, lazy_arrays, writes);
    walk_statements(&stmt->else_body, lazy_arrays, writes);
}

/** Percorre uma lista de comandos. */
static void walk_statements(StatementList *statements, VariableSet *lazy_arrays,
                            VariableSet *writes)
{
    for (int i = 0; i < statements->count; i++)
    {
        walk_statement(statements->items[i], lazy_arrays, writes);
    }
}

/* o que cada função altera, repetindo até estabilizar (recursão mútua) */
static void compute_writes(Program *parsed_program)
{
    int changed = 1;

    while (changed)
    {
        changed = 0;

        for (int i = 0; i < parsed_program->instances.count; i++)
        {
            FunctionInstance *instance = parsed_program->instances.items[i];
            VariableSet writes = {0};

            walk_statements(&instance->body, NULL, &writes);

            for (int j = 0; j < writes.count; j++)
            {
                if (set_add(&instance->writes, writes.items[j]))
                {
                    changed = 1;
                }
            }

            free(writes.items);
        }
    }
}

/** Diz se o tamanho do #decl pode mudar dentro do trecho (aí a checagem fica no laço). */
static int dimensions_change_in(Variable *variable, VariableSet *writes)
{
    VariableSet names = {0};
    int changes = 0;

    /* variáveis citadas no tamanho */
    for (int i = 0; i < variable->dimensions.count; i++)
    {
        Expression *stack[64];
        int top = 0;
        stack[top++] = variable->dimensions.items[i];

        while (top)
        {
            Expression *expr = stack[--top];

            if (!expr)
            {
                continue;
            }

            if (expr->kind == EXPR_NAME)
            {
                set_add(&names, expr->variable);
            }

            if (expr->kind == EXPR_CALL)
            {
                changes = 1; /* tamanho que chama função: não arrisca */
            }

            if (top + 3 + expr->arguments.count <= 64)
            {
                stack[top++] = expr->first;
                stack[top++] = expr->second;
                stack[top++] = expr->third;

                for (int j = 0; j < expr->arguments.count; j++)
                {
                    stack[top++] = expr->arguments.items[j];
                }
            }
            else
            {
                changes = 1;
            }
        }
    }

    for (int i = 0; i < names.count && !changes; i++)
    {
        changes = set_contains(writes, names.items[i]);
    }

    free(names.items);
    return changes;
}

static void gen_loop(TextBuffer *out, Statement *stmt, int depth);
static void gen_for_in(TextBuffer *out, Statement *stmt, int depth);

/** Escreve um laço com a checagem do #decl feita uma vez antes dele, quando é seguro. */
static void gen_loop_hoisted(TextBuffer *out, Statement *stmt, int depth)
{
    VariableSet lazy_arrays = {0};
    VariableSet writes = {0};
    int saved_count = ensured_lazy.count;

    walk_statement(stmt, &lazy_arrays, &writes);

    for (int i = 0; i < lazy_arrays.count; i++)
    {
        Variable *variable = lazy_arrays.items[i];

        if (set_contains(&ensured_lazy, variable) || dimensions_change_in(variable, &writes))
        {
            continue;
        }

        indent(out, depth);
        text_append(out, "if (!");
        emit_identifier(out, "v_", variable->name);
        text_append(out, ".data)\n");
        indent(out, depth);
        text_append(out, "{\n");
        indent(out, depth + 1);
        emit_identifier(out, "v_", variable->name);
        text_append(out, " = ");
        emit_lazy_allocation(out, variable, stmt->line);
        text_append(out, ";\n");
        indent(out, depth);
        text_append(out, "}\n");
        PUSH(ensured_lazy, variable);
    }

    gen_loop(out, stmt, depth);
    ensured_lazy.count = saved_count;
    free(lazy_arrays.items);
    free(writes.items);
}

/* var = valor (uma linha, já indentada) */
static void gen_assign_line(TextBuffer *out, Variable *variable, int line, int depth,
                            const char *value_format, ...)
{
    char value[256];
    va_list arguments;

    va_start(arguments, value_format);
    vsnprintf(value, sizeof value, value_format, arguments);
    va_end(arguments);

    indent(out, depth);
    gen_variable(out, variable, line);
    text_appendf(out, " = %s;\n", value);
}

/*
 * para ... em: vetor (elementos ou linhas), mapa (chaves em ordem) ou
 * g.vizinhos(u). Com (a, b), cada parte vai para uma variável.
 */
static void gen_for_in(TextBuffer *out, Statement *stmt, int depth)
{
    int temp_id = temp_counter++;
    Type *iterable_type = stmt->first->type;
    int body_indent = depth + 2;

    indent(out, depth);
    text_append(out, "{\n");
    indent(out, depth + 1);

    if (stmt->first->kind == EXPR_METHOD && iterable_type->kind == TYPE_GRAPH)
    {
        const char *c_name = template_name(iterable_type);

        text_appendf(out, "%s *_g%d = ", template_type(iterable_type), temp_id);
        gen_expression(out, stmt->first->first);
        text_append(out, ";\n");
        indent(out, depth + 1);
        text_appendf(out, "int64_t _u%d = ", temp_id);
        gen_expression(out, stmt->first->arguments.items[0]);
        text_append(out, ";\n");
        indent(out, depth + 1);
        text_appendf(out, "%s_ready(_g%d);\n", c_name, temp_id);
        indent(out, depth + 1);
        text_appendf(out, "check_index(_u%d, _g%d->vertex_count, %d);\n", temp_id, temp_id,
                     stmt->line);
        indent(out, depth + 1);
        text_appendf(out,
                     "for (int64_t _e%d = _g%d->adjacency_start[_u%d]; _e%d < "
                     "_g%d->adjacency_start[_u%d + 1]; _e%d++)\n",
                     temp_id, temp_id, temp_id, temp_id, temp_id, temp_id, temp_id);
        indent(out, depth + 1);
        text_append(out, "{\n");

        if (stmt->pattern_variables)
        {
            gen_assign_line(out, stmt->pattern_variables[0], stmt->line, body_indent,
                            "_g%d->adjacency[_e%d]", temp_id, temp_id);
            gen_assign_line(out, stmt->pattern_variables[1], stmt->line, body_indent,
                            "_g%d->adjacency_weight[_e%d]", temp_id, temp_id);
        }
        else
        {
            gen_assign_line(out, stmt->variable, stmt->line, body_indent, "_g%d->adjacency[_e%d]",
                            temp_id, temp_id);
        }
    }
    else if (iterable_type->kind == TYPE_MAP)
    {
        const char *c_name = template_name(iterable_type);

        text_appendf(out, "%s *_m%d = ", template_type(iterable_type), temp_id);
        gen_expression(out, stmt->first);
        text_append(out, ";\n");
        indent(out, depth + 1);
        text_appendf(out, "Array%s _a%d = %s_keys(_m%d);\n",
                     cell_element_kind(iterable_type->element)->pascal, temp_id, c_name, temp_id);
        indent(out, depth + 1);
        text_appendf(out, "for (int64_t _k%d = 0; _k%d < _a%d.length; _k%d++)\n", temp_id, temp_id,
                     temp_id, temp_id);
        indent(out, depth + 1);
        text_append(out, "{\n");

        if (stmt->pattern_variables)
        {
            gen_assign_line(out, stmt->pattern_variables[0], stmt->line, body_indent,
                            "_a%d.data[_k%d]", temp_id, temp_id);
            gen_assign_line(out, stmt->pattern_variables[1], stmt->line, body_indent,
                            "%s_get(_m%d, _a%d.data[_k%d])", c_name, temp_id, temp_id, temp_id);
        }
        else
        {
            gen_assign_line(out, stmt->variable, stmt->line, body_indent, "_a%d.data[_k%d]",
                            temp_id, temp_id);
        }
    }
    else
    {
        const char *suffix = array_suffix(iterable_type);

        text_appendf(out, "Array%s _a%d = ", array_pascal(iterable_type), temp_id);
        gen_expression(out, stmt->first);
        text_append(out, ";\n");
        indent(out, depth + 1);
        text_appendf(out, "for (int64_t _k%d = 0; _k%d < _a%d.length; _k%d++)\n", temp_id, temp_id,
                     temp_id, temp_id);
        indent(out, depth + 1);
        text_append(out, "{\n");

        if (iterable_type->dimension_count > 1)
        {
            gen_assign_line(out, stmt->variable, stmt->line, body_indent,
                            "array_%s_row(_a%d, _k%d, %d)", suffix, temp_id, temp_id, stmt->line);
        }
        else if (stmt->pattern_variables)
        {
            for (int i = 0; i < stmt->parameters.count; i++)
            {
                gen_assign_line(out, stmt->pattern_variables[i], stmt->line, body_indent,
                                "_a%d.data[_k%d].field_%d", temp_id, temp_id, i);
            }
        }
        else
        {
            gen_assign_line(out, stmt->variable, stmt->line, body_indent, "%s(_a%d.data[_k%d])",
                            cell_element_kind(iterable_type->element)->loader, temp_id, temp_id);
        }
    }

    gen_statements(out, &stmt->body, body_indent);
    indent(out, depth + 1);
    text_append(out, "}\n");
    indent(out, depth);
    text_append(out, "}\n");
}

/* (a, b) = tupla */
static void gen_tuple_assignment(TextBuffer *out, Statement *stmt, int depth)
{
    int temp_id = temp_counter++;

    indent(out, depth);
    text_append(out, "{\n");
    indent(out, depth + 1);
    emit_c_type(out, stmt->second->type);
    text_appendf(out, " _t%d = ", temp_id);
    gen_expression(out, stmt->second);
    text_append(out, ";\n");

    for (int i = 0; i < stmt->first->arguments.count; i++)
    {
        Expression *item = stmt->first->arguments.items[i];

        indent(out, depth + 1);

        if (item->kind == EXPR_NAME)
        {
            if (item->variable->is_lazy)
            {
                emit_identifier(out, "v_", item->variable->name);
            }
            else
            {
                gen_variable(out, item->variable, item->line);
            }

            text_appendf(out, " = _t%d.field_%d;\n", temp_id, i);
            continue;
        }

        gen_lvalue(out, item);

        if (item->first->type->kind == TYPE_ARRAY && is_checked_int32(item->type))
        {
            text_appendf(out, " = %s(_t%d.field_%d, %d);\n",
                         program->uses_inf ? "encode_int32_inf" : "fit_int32", temp_id, i,
                         item->line);
        }
        else
        {
            text_appendf(out, " = _t%d.field_%d;\n", temp_id, i);
        }
    }

    indent(out, depth);
    text_append(out, "}\n");
}

/* para / para-em / enquanto */
static void gen_loop(TextBuffer *out, Statement *stmt, int depth)
{
    if (stmt->kind == STMT_WHILE)
    {
        indent(out, depth);
        text_append(out, "while (");
        gen_expression(out, stmt->first);
        text_append(out, ")\n");
        gen_block(out, &stmt->body, depth);
        return;
    }

    if (stmt->kind == STMT_FOR)
    {
        int temp_id = temp_counter++;
        indent(out, depth);
        text_append(out, "{\n");
        indent(out, depth + 1);
        text_appendf(out, "int64_t _h%d = ", temp_id);
        gen_expression(out, stmt->second);
        text_append(out, ";\n");
        indent(out, depth + 1);
        text_append(out, "for (");
        gen_variable(out, stmt->variable, stmt->line);
        text_append(out, " = ");
        gen_expression(out, stmt->first);
        text_append(out, "; ");
        gen_variable(out, stmt->variable, stmt->line);
        text_appendf(out, " %s _h%d; ", stmt->inclusive ? "<=" : "<", temp_id);
        gen_variable(out, stmt->variable, stmt->line);
        text_append(out, "++)\n");
        gen_block(out, &stmt->body, depth + 1);
        indent(out, depth);
        text_append(out, "}\n");
        return;
    }

    gen_for_in(out, stmt, depth);
}

/** Escreve um comando em C. */
static void gen_statement(TextBuffer *out, Statement *stmt, int depth)
{
    switch (stmt->kind)
    {
    case STMT_FUNCTION:
    case STMT_DECL:
        return;

    case STMT_EXPR:
    {
        Expression *expr = stmt->first;
        Builtin builtin = expr->kind == EXPR_CALL && !expr->instance
                              ? builtin_of(expr->text, LANG_PT)
                              : BUILTIN_NONE;

        if (expr->kind == EXPR_CALL && !expr->instance && !builtin)
        {
            builtin = builtin_of(expr->text, LANG_EN);
        }

        if (builtin == BUILTIN_LER)
        {
            for (int i = 0; i < expr->arguments.count; i++)
            {
                gen_read(out, expr->arguments.items[i], depth);
            }

            return;
        }

        if (builtin == BUILTIN_ESCREVER)
        {
            gen_write(out, expr, depth);
            return;
        }

        indent(out, depth);
        gen_expression(out, expr);
        text_append(out, ";\n");
        return;
    }

    case STMT_ASSIGN:
        if (stmt->first->kind == EXPR_TUPLE)
        {
            gen_tuple_assignment(out, stmt, depth);
            return;
        }

        indent(out, depth);

        if (stmt->first->kind == EXPR_NAME && stmt->first->variable->is_lazy)
        {
            emit_identifier(out, "v_", stmt->first->variable->name);
            text_append(out, " = ");
            gen_expression(out, stmt->second);
        }
        else if (stmt->first->kind == EXPR_INDEX)
        {
            gen_lvalue(out, stmt->first);
            text_append(out, " = ");
            gen_stored_value(out, stmt->first->type, stmt->second);
        }
        else
        {
            gen_expression(out, stmt->first);
            text_append(out, " = ");
            gen_expression(out, stmt->second);
        }

        text_append(out, ";\n");
        return;

    case STMT_IF:
        indent(out, depth);
        text_append(out, "if (");
        gen_expression(out, stmt->first);
        text_append(out, ")\n");
        gen_block(out, &stmt->body, depth);

        if (stmt->else_body.count)
        {
            indent(out, depth);
            text_append(out, "else\n");
            gen_block(out, &stmt->else_body, depth);
        }

        return;

    case STMT_WHILE:
    case STMT_FOR:
    case STMT_FOR_IN:
        gen_loop_hoisted(out, stmt, depth);
        return;

    case STMT_BREAK:
        indent(out, depth);
        text_append(out, "break;\n");
        return;

    case STMT_CONTINUE:
        indent(out, depth);
        text_append(out, "continue;\n");
        return;

    case STMT_RETURN:
        indent(out, depth);

        if (stmt->first)
        {
            text_append(out, "return ");
            gen_expression(out, stmt->first);
            text_append(out, ";\n");
        }
        else
        {
            text_append(out, "return;\n");
        }

        return;
    }
}

/** Escreve uma lista de comandos. */
static void gen_statements(TextBuffer *out, StatementList *statements, int depth)
{
    for (int i = 0; i < statements->count; i++)
    {
        gen_statement(out, statements->items[i], depth);
    }
}

/* ---------- programa ---------- */

static void gen_prototype(TextBuffer *out, FunctionInstance *instance)
{
    text_append(out, "static ");
    emit_c_type(out, instance->return_type);
    text_append(out, " ");
    emit_identifier(out, "f_", instance->c_name);
    text_append(out, "(");

    for (int i = 0; i < instance->function->definition->parameters.count; i++)
    {
        if (i)
        {
            text_append(out, ", ");
        }

        emit_c_type(out, instance->parameters[i]->type);
        text_append(out, instance->parameters[i]->type->kind == TYPE_ARRAY ? " *" : " ");
        emit_identifier(out, "v_", instance->parameters[i]->name);
    }

    if (!instance->function->definition->parameters.count)
    {
        text_append(out, "void");
    }

    text_append(out, ")");
}

/** Escreve a declaração (zerada) de uma variável. */
static void gen_variable_declaration(TextBuffer *out, Variable *variable, int depth,
                                     const char *prefix)
{
    indent(out, depth);
    text_append(out, prefix);
    emit_c_type(out, variable->type);
    text_append(out, " ");
    emit_identifier(out, "v_", variable->name);
    text_append(out, variable->type->kind == TYPE_ARRAY || variable->type->kind == TYPE_TUPLE
                         ? " = {0};\n"
                         : " = 0;\n");
}

/* struct da tupla e suas funções: ordem, igualdade, hash, escrita, leitura */
static void emit_tuple(TextBuffer *out, const ElementKind *kind)
{
    const char *signature = kind->signature;
    const char *type = kind->c_type;
    const char *prefix = kind->suffix;
    int count = (int)strlen(signature);

    text_append(out, "\ntypedef struct\n{\n");

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    %s field_%d;\n", signature[i] == 'r' ? "double" : "int64_t", i);
    }

    text_appendf(out, "} %s;\n\n", type);

    text_appendf(out, "static inline int %s_less(%s left, %s right)\n{\n", prefix, type, type);

    for (int i = 0; i < count - 1; i++)
    {
        text_appendf(out,
                     "    if (left.field_%d != right.field_%d)\n    {\n"
                     "        return left.field_%d < right.field_%d;\n    }\n\n",
                     i, i, i, i);
    }

    text_appendf(out, "    return left.field_%d < right.field_%d;\n}\n\n", count - 1, count - 1);

    text_appendf(out, "static inline int %s_equal(%s left, %s right)\n{\n    return 1", prefix,
                 type, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, " && left.field_%d == right.field_%d", i, i);
    }

    text_append(out, ";\n}\n\n");

    text_appendf(out, "static inline uint64_t %s_hash(%s tuple)\n{\n    uint64_t hash = 0;\n",
                 prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    hash = mix_bits(hash ^ %s(tuple.field_%d));\n",
                     signature[i] == 'r' ? "hash_real" : "hash_int", i);
    }

    text_append(out, "    return hash;\n}\n\n");

    text_appendf(out, "static void %s_write(%s tuple)\n{\n", prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "%s    %s(tuple.field_%d);\n", i ? "    write_space();\n" : "",
                     signature[i] == 'r' ? "write_real" : "write_int", i);
    }

    text_append(out, "}\n\n");

    text_appendf(out, "static %s %s_read(int line)\n{\n    %s tuple;\n", type, prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    tuple.field_%d = %s(line);\n", i,
                     signature[i] == 'r' ? "read_real" : "read_int");
    }

    text_append(out, "    return tuple;\n}\n");
}

/** Copia um arquivo do runtime embutido para a saída. */
static void emit_text(TextBuffer *out, const unsigned char *text, unsigned int length)
{
    text_appendf(out, "%.*s", (int)length, (const char *)text);
}

/** Corpo do macro LOAD: como um elemento guardado vira valor da linguagem. */
static const char *load_macro(const ElementKind *kind)
{
    if (!*kind->loader)
    {
        return "(stored)";
    }

    return !strcmp(kind->loader, "(int64_t)") ? "((int64_t)(stored))" : "decode_int32_inf(stored)";
}

/** Escreve o template de vetor para um tipo de elemento. */
static void emit_array_template(TextBuffer *out, const ElementKind *kind)
{
    text_appendf(out, "\n#define ELEMENT %s\n#define VALUE %s\n", kind->c_type, kind->value_type);
    text_appendf(out, "#define ARRAY Array%s\n#define FN(name) array_%s##name\n", kind->pascal,
                 kind->suffix);
    text_appendf(out, "#define READ_ELEMENT %s\n#define WRITE_ELEMENT %s\n", kind->reader,
                 kind->writer);
    text_appendf(out, "#define LESS(left, right) %s(left, right)\n", kind->less_than);
    text_appendf(out, "#define LOAD(stored) %s\n#define KIND %d\n", load_macro(kind), kind->kind);
    emit_text(out, rt_arr_h, rt_arr_h_len);
    text_append(out, "#undef ELEMENT\n#undef VALUE\n#undef ARRAY\n#undef FN\n#undef READ_ELEMENT\n"
                     "#undef WRITE_ELEMENT\n#undef LESS\n#undef LOAD\n#undef KIND\n");
}

/** Escreve o template de uma estrutura (heap, fila, mapa ou grafo). */
static void emit_template(TextBuffer *out, const TemplateUse *template_use)
{
    const ElementKind *element_kind = template_use->element;

    text_appendf(out, "\n#define FN(name) %s##name\n", template_use->prefix);

    if (template_use->kind == TEMPLATE_HEAP)
    {
        text_appendf(out, "#define ELEMENT %s\n#define HEAP %s\n", element_kind->c_type,
                     template_use->type_name);
        text_appendf(out, "#define LESS(left, right) %s(%s, %s)\n", element_kind->less_than,
                     template_use->descending ? "right" : "left",
                     template_use->descending ? "left" : "right");
        emit_text(out, rt_heap_h, rt_heap_h_len);
        text_append(out, "#undef ELEMENT\n#undef HEAP\n#undef LESS\n");
    }
    else if (template_use->kind == TEMPLATE_QUEUE)
    {
        text_appendf(out, "#define ELEMENT %s\n#define QUEUE %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_queue_h, rt_queue_h_len);
        text_append(out, "#undef ELEMENT\n#undef QUEUE\n");
    }
    else if (template_use->kind == TEMPLATE_MAP)
    {
        const char *hash_macro = element_kind->kind == 2
                                     ? format_string("%s_hash(key)", element_kind->suffix)
                                 : element_kind->kind == 1 ? "hash_real(key)"
                                                           : "hash_int(key)";
        const char *equal_macro = element_kind->kind == 2
                                      ? format_string("%s_equal(left, right)", element_kind->suffix)
                                      : "((left) == (right))";

        text_appendf(out, "#define KEY %s\n#define MAP_VALUE %s\n#define MAP %s\n",
                     element_kind->c_type, template_use->value->c_type, template_use->type_name);
        text_appendf(out, "#define HASH(key) %s\n#define KEYS_EQUAL(left, right) %s\n", hash_macro,
                     equal_macro);
        text_appendf(out, "#define KEY_ARRAY Array%s\n#define KEY_ARRAY_FN(name) array_%s##name\n",
                     element_kind->pascal, element_kind->suffix);
        emit_text(out, rt_map_h, rt_map_h_len);
        text_append(out,
                    "#undef KEY\n#undef MAP_VALUE\n#undef MAP\n#undef HASH\n#undef KEYS_EQUAL\n"
                    "#undef KEY_ARRAY\n#undef KEY_ARRAY_FN\n");
    }
    else if (template_use->kind == TEMPLATE_FENWICK)
    {
        text_appendf(out, "#define ELEMENT %s\n#define FENWICK %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_fenwick_h, rt_fenwick_h_len);
        text_append(out, "#undef ELEMENT\n#undef FENWICK\n");
    }
    else if (template_use->kind == TEMPLATE_SEGTREE || template_use->kind == TEMPLATE_RMQ)
    {
        static const char *const COMBINE_C[] = {"((left) < (right) ? (left) : (right))",
                                                "((left) > (right) ? (left) : (right))",
                                                "((left) + (right))"};
        int is_real = element_kind->kind == 1;
        const char *identity = template_use->descending == COMBINE_SUM ? "0"
                               : template_use->descending == COMBINE_MIN
                                   ? (is_real ? "INFINITY" : "INT64_MAX")
                                   : (is_real ? "-INFINITY" : "INT64_MIN");
        int is_rmq = template_use->kind == TEMPLATE_RMQ;

        text_appendf(out, "#define ELEMENT %s\n#define %s %s\n", element_kind->c_type,
                     is_rmq ? "RMQ" : "SEGTREE", template_use->type_name);
        text_appendf(out, "#define COMBINE(left, right) %s\n#define IDENTITY %s\n",
                     COMBINE_C[template_use->descending], identity);

        if (is_rmq)
        {
            emit_text(out, rt_rmq_h, rt_rmq_h_len);
        }
        else
        {
            emit_text(out, rt_segtree_h, rt_segtree_h_len);
        }

        text_appendf(out, "#undef ELEMENT\n#undef %s\n#undef COMBINE\n#undef IDENTITY\n",
                     is_rmq ? "RMQ" : "SEGTREE");
    }
    else
    {
        text_appendf(out, "#define WEIGHT %s\n#define GRAPH %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_graph_h, rt_graph_h_len);
        text_append(out, "#undef WEIGHT\n#undef GRAPH\n");
    }

    text_append(out, "#undef FN\n");
}

/* variável do programa principal citada em alguma função precisa ser global no C */
static void mark_expression(Expression *expr)
{
    if (!expr)
    {
        return;
    }

    if (expr->kind == EXPR_NAME && expr->variable && !expr->variable->used_in_function)
    {
        expr->variable->used_in_function = 1;

        /* o tamanho de um #decl é calculado onde o vetor é usado */
        for (int i = 0; i < expr->variable->dimensions.count; i++)
        {
            mark_expression(expr->variable->dimensions.items[i]);
        }
    }

    mark_expression(expr->first);
    mark_expression(expr->second);
    mark_expression(expr->third);

    for (int i = 0; i < expr->arguments.count; i++)
    {
        mark_expression(expr->arguments.items[i]);
    }
}

/** Marca as variáveis usadas dentro de funções (essas ficam globais no C). */
static void mark_statements(StatementList *statements)
{
    for (int i = 0; i < statements->count; i++)
    {
        Statement *stmt = statements->items[i];

        if (stmt->variable)
        {
            stmt->variable->used_in_function = 1;
        }

        mark_expression(stmt->first);
        mark_expression(stmt->second);
        mark_expression(stmt->third);
        mark_statements(&stmt->body);
        mark_statements(&stmt->else_body);
    }
}

/*
 * Com -O, pede ao gcc otimizações extras de dentro do próprio arquivo:
 * assim valem também quando o juiz compila o C gerado com as flags dele.
 */
static const char *PRAGMAS = "#pragma GCC optimize(\"O3,unroll-loops\")\n"
                             "#pragma GCC target(\"avx2\")\n";

/** Gera o programa C completo: runtime, templates usados, funções e alg_main. */
void generate_c(Program *parsed_program, TextBuffer *out)
{
    TextBuffer prototypes = {0};
    TextBuffer function_bodies = {0};
    TextBuffer main_body = {0};

    program = parsed_program;
    compute_writes(parsed_program);

    for (int i = 0; i < parsed_program->instances.count; i++)
    {
        mark_statements(&parsed_program->instances.items[i]->body);
    }

    /* primeiro gera o programa: só então se sabe quais templates de vetor ele usa */
    for (int i = 0; i < parsed_program->instances.count; i++)
    {
        FunctionInstance *instance = parsed_program->instances.items[i];
        gen_prototype(&prototypes, instance);
        text_append(&prototypes, ";\n");

        text_append(&function_bodies, "\n");
        gen_prototype(&function_bodies, instance);
        text_append(&function_bodies, "\n{\n");

        for (int j = 0; j < instance->locals.count; j++)
        {
            gen_variable_declaration(&function_bodies, instance->locals.items[j], 1, "");
        }

        gen_statements(&function_bodies, &instance->body, 1);
        text_append(&function_bodies, "}\n");
    }

    text_append(&main_body, "\nstatic void alg_main(void)\n{\n");

    /* variável que nenhuma função usa vira local do main: o gcc otimiza melhor */
    for (int i = 0; i < parsed_program->globals.count; i++)
    {
        if (!parsed_program->globals.items[i]->used_in_function)
        {
            gen_variable_declaration(&main_body, parsed_program->globals.items[i], 1, "");
        }
    }

    gen_statements(&main_body, &parsed_program->statements, 1);
    text_append(&main_body, "}\n");

    TextBuffer global_declarations = {0};

    for (int i = 0; i < parsed_program->globals.count; i++)
    {
        if (parsed_program->globals.items[i]->used_in_function)
        {
            gen_variable_declaration(&global_declarations, parsed_program->globals.items[i], 0,
                                     "static ");
        }
    }

    if (parsed_program->no_checks)
    {
        text_append(out, PRAGMAS);
    }

    text_appendf(out, "%.*s", (int)rt_core_h_len, (const char *)rt_core_h);

    for (int i = 0; i < tuples.count; i++)
    {
        emit_tuple(out, tuples.items[i]);
    }

    for (int kind_index = 0; kind_index < ELEMENT_KIND_COUNT; kind_index++)
    {
        if (element_used[kind_index])
        {
            emit_array_template(out, &ELEMENT_KINDS[kind_index]);
        }
    }

    for (int i = 0; i < tuples.count; i++)
    {
        emit_array_template(out, tuples.items[i]);
    }

    for (int i = 0; i < templates.count; i++)
    {
        emit_template(out, &templates.items[i]);
    }

    text_append(out, "\n/* ===== programa ===== */\n\n");

    TextBuffer *parts[] = {&global_declarations, &prototypes, &function_bodies, &main_body};

    for (int i = 0; i < 4; i++)
    {
        if (parts[i]->text)
        {
            text_append(out, parts[i]->text);
            text_append(out, i < 2 ? "\n" : "");
        }
    }
}
