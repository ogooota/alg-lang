#include "gen_internal.h"

/* ---------- nomes e tipos ---------- */

void indent(TextBuffer *out, int depth)
{
    for (int i = 0; i < depth; i++)
    {
        text_append(out, "    ");
    }
}

/* identificador do usuário → C: prefixo + bytes não-ASCII como _xx */
void emit_identifier(TextBuffer *out, const char *prefix, const char *name)
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
Type *resolve_unknown_type(Type *type)
{
    if (type->kind == TYPE_UNKNOWN)
    {
        type->kind = TYPE_INT;
    }

    return type;
}

/** Texto formatado com um argumento, em memória nova. */
char *format_string(const char *format_text, const char *argument)
{
    char *text = allocate_zeroed(strlen(format_text) + strlen(argument) + 1);
    sprintf(text, format_text, argument);
    return text;
}

/** Descrição C de um formato de tupla (registra a struct na primeira vez). */
const ElementKind *tuple_element_kind(Type *type)
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
int scalar_element_kind(Type *cell)
{
    if (cell->kind == TYPE_REAL)
    {
        return ELEMENT_REAL;
    }

    if (cell->is_int32)
    {
        return generated_program->uses_inf ? ELEMENT_I32INF : ELEMENT_I32;
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
const ElementKind *cell_element_kind(Type *cell)
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
const char *array_suffix(Type *array_type)
{
    return cell_element_kind(array_type->element)->suffix;
}

/** Nome do tipo C do vetor: ArrayU8, ArrayI64, ArrayTupleIntInt... */
const char *array_pascal(Type *array_type)
{
    return cell_element_kind(array_type->element)->pascal;
}

/* ': int32' anotado: o valor guardado é checado (sem -O) */
int is_checked_int32(Type *cell)
{
    return cell_root(cell)->is_int32;
}

/** Registra (uma vez) a instância de um template de estrutura e a devolve. */
const TemplateUse *use_template(int kind, const ElementKind *element_kind,
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
const TemplateUse *structure_template(Type *type)
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
const char *template_name(Type *type)
{
    return structure_template(type)->prefix;
}

/** Nome do tipo C da estrutura: HeapI64Min, MapI64I64, GraphI64... */
const char *template_type(Type *type)
{
    return structure_template(type)->type_name;
}

/** Diz se o tipo é heap, fila, mapa ou grafo (ponteiro no C). */
int is_structure_type(Type *type)
{
    return type->kind == TYPE_HEAP || type->kind == TYPE_QUEUE || type->kind == TYPE_MAP ||
           type->kind == TYPE_GRAPH || type->kind == TYPE_DSU || type->kind == TYPE_FENWICK ||
           type->kind == TYPE_SEGTREE || type->kind == TYPE_RMQ;
}

/** Escreve o tipo C de um tipo da linguagem. */
void emit_c_type(TextBuffer *out, Type *type)
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
void emit_c_string(TextBuffer *out, const char *text)
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
