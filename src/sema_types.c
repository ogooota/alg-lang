#include "sema_internal.h"

/** Novo tipo do tipo básico pedido. */
Type *new_type(TypeKind kind)
{
    Type *type = allocate_zeroed(sizeof *type);
    type->kind = kind;
    return type;
}

/** Tipo de vetor com o elemento e o número de dimensões dados. */
Type *array_type(Type *elem, int nd)
{
    Type *type = new_type(TYPE_ARRAY);
    type->element = elem;
    type->dimension_count = nd;
    return type;
}

/* representante da célula de elemento (union-find com compressão de caminho) */
Type *cell_root(Type *type)
{
    Type *root = type;

    while (root->link)
    {
        root = root->link;
    }

    while (type->link)
    {
        Type *next = type->link;
        type->link = root;
        type = next;
    }

    return root;
}

/* valor lido sem tipo conhecido vira inteiro */
Type *resolve_unknown(Type *type)
{
    if (type->kind == TYPE_UNKNOWN)
    {
        type->kind = TYPE_INT;
    }

    return type;
}

/** Diz se o tipo é inteiro ou real. */
int is_number(Type *type)
{
    return type->kind == TYPE_INT || type->kind == TYPE_REAL;
}

/** Nome legível de um tipo, para mensagens de erro. */
const char *type_name(Type *type)
{
    static char slots[4][64];
    static int next_slot;
    char *slot = slots[next_slot++ % 4];

    switch (type->kind)
    {
    case TYPE_UNKNOWN:
        return "desconhecido";

    case TYPE_INT:
        return "inteiro";

    case TYPE_REAL:
        return "real";

    case TYPE_STRING:
        return "texto";

    case TYPE_VOID:
        return "nada";

    case TYPE_ARRAY:
        snprintf(slot, 64, "vetor %dD de %s", type->dimension_count, type_name(type->element));
        return slot;

    case TYPE_TUPLE:
        snprintf(slot, 64, "tupla de %d valores", type->field_count);
        return slot;

    case TYPE_HEAP:
        return "heap";

    case TYPE_QUEUE:
        return "fila";

    case TYPE_GRAPH:
        return "grafo";

    case TYPE_MAP:
        return "mapa";

    case TYPE_DSU:
        return "conjuntos";

    case TYPE_FENWICK:
        return "fenwick";

    case TYPE_SEGTREE:
        return "segtree";

    case TYPE_RMQ:
        return "rmq";
    }

    return "?";
}

/** Diz se duas tuplas têm o mesmo número de campos com os mesmos tipos. */
int same_tuple(Type *left_type, Type *right_type)
{
    if (left_type->field_count != right_type->field_count)
    {
        return 0;
    }

    for (int i = 0; i < left_type->field_count; i++)
    {
        if (left_type->fields[i]->kind != right_type->fields[i]->kind)
        {
            return 0;
        }
    }

    return 1;
}

/* célula ainda sem tipo recebe o tipo de t (escalar ou tupla) */
void fill_cell(Type *cell, Type *type)
{
    cell->kind = type->kind;
    cell->fields = type->fields;
    cell->field_count = type->field_count;
}

/* para variáveis novas: escalar vira tipo próprio (não compartilha célula); vetor compartilha */
Type *owned_type(Type *type)
{
    resolve_unknown(type);

    if (type->kind == TYPE_INT)
    {
        return &INT_TYPE;
    }

    if (type->kind == TYPE_REAL)
    {
        return &REAL_TYPE;
    }

    if (type->kind == TYPE_TUPLE)
    {
        Type *cell = new_type(TYPE_TUPLE);
        cell->fields = type->fields;
        cell->field_count = type->field_count;
        return cell;
    }

    return type;
}

/* elementos de vetor: o tipo precisa ser exato (o layout na memória depende dele) */
void unify_elements(Type *target, Type *source, Expression *location)
{
    if (target == source)
    {
        return;
    }

    if (target->kind == TYPE_UNKNOWN && source->kind == TYPE_UNKNOWN)
    {
        target->kind = TYPE_INT;
        source->kind = TYPE_INT;
    }
    else if (target->kind == TYPE_UNKNOWN)
    {
        fill_cell(target, source);
    }
    else if (source->kind == TYPE_UNKNOWN)
    {
        fill_cell(source, target);
    }
    else if (target->kind != source->kind ||
             (target->kind == TYPE_TUPLE && !same_tuple(target, source)))
    {
        FAIL_AT_NODE(location, "vetor de %s não combina com vetor de %s", type_name(target),
                     type_name(source));
    }

    /* os dois vetores compartilham dados: precisam do mesmo tamanho de elemento */
    Type *target_root = cell_root(target);
    Type *source_root = cell_root(source);

    if (target_root != source_root)
    {
        target_root->link = source_root;
        source_root->is_int32 |= target_root->is_int32;
        source_root->is_structure_cell |= target_root->is_structure_cell;
    }
}

/* atribuir valor do tipo s num lugar do tipo d (inteiro → real é permitido) */
void unify_types(Type *target, Type *source, Expression *location)
{
    if (target == source)
    {
        return;
    }

    if (source->kind == TYPE_UNKNOWN)
    {
        resolve_unknown(source);
    }

    if (target->kind == TYPE_UNKNOWN)
    {
        *target = *source;
        return;
    }

    if (target->kind == TYPE_REAL && source->kind == TYPE_INT)
    {
        return;
    }

    if (target->kind != source->kind)
    {
        FAIL_AT_NODE(location, "esperava %s, mas o valor é %s", type_name(target),
                     type_name(source));
    }

    if (target->kind == TYPE_ARRAY)
    {
        if (target->dimension_count != source->dimension_count)
        {
            FAIL_AT_NODE(location, "esperava %s, mas o valor é %s", type_name(target),
                         type_name(source));
        }

        unify_elements(target->element, source->element, location);
    }

    if (target->kind == TYPE_TUPLE && !same_tuple(target, source))
    {
        FAIL_AT_NODE(location, "as tuplas não combinam (%d e %d valores, ou tipos diferentes)",
                     target->field_count, source->field_count);
    }

    if (target->kind == TYPE_HEAP && target->descending != source->descending)
    {
        FAIL_AT_NODE(location, "heap() e heap(maior) são tipos diferentes");
    }

    if (target->kind == TYPE_GRAPH && target->directed != source->directed)
    {
        FAIL_AT_NODE(location, "grafo dirigido e não dirigido são tipos diferentes");
    }

    if ((target->kind == TYPE_SEGTREE || target->kind == TYPE_RMQ) &&
        target->combine != source->combine)
    {
        FAIL_AT_NODE(location,
                     "estruturas com operações diferentes (min, max, soma) são tipos diferentes");
    }

    if (target->kind == TYPE_FENWICK || target->kind == TYPE_SEGTREE || target->kind == TYPE_RMQ)
    {
        unify_elements(target->element, source->element, location);
    }

    if (target->kind == TYPE_HEAP || target->kind == TYPE_QUEUE || target->kind == TYPE_MAP ||
        target->kind == TYPE_GRAPH)
    {
        unify_elements(target->element, source->element, location);
    }

    if (target->kind == TYPE_MAP)
    {
        unify_elements(target->value, source->value, location);
    }

    if (target->kind == TYPE_GRAPH)
    {
        target->weighted |= source->weighted;
        source->weighted = target->weighted;
    }
}

/*
 * Guardar um valor do tipo t numa célula de estrutura (heap, fila, mapa, peso):
 * célula vazia recebe o tipo; real aceita inteiro; tupla precisa ser igual.
 */
void store_in_cell(Type *cell, Type *type, Expression *location)
{
    if (type->kind == TYPE_UNKNOWN)
    {
        type->kind = TYPE_INT;
    }

    if (type->kind != TYPE_INT && type->kind != TYPE_REAL && type->kind != TYPE_TUPLE)
    {
        FAIL_AT_NODE(location, "não dá para guardar %s aqui", type_name(type));
    }

    if (cell->kind == TYPE_UNKNOWN)
    {
        fill_cell(cell, type);
        return;
    }

    if (cell->kind == TYPE_REAL && type->kind == TYPE_INT)
    {
        return;
    }

    if (cell->kind != type->kind || (type->kind == TYPE_TUPLE && !same_tuple(cell, type)))
    {
        FAIL_AT_NODE(location, "esperava %s, mas o valor é %s", type_name(cell), type_name(type));
    }
}

/** Célula de elemento para heap, fila, mapa ou grafo (nunca estreitada). */
Type *new_structure_cell(void)
{
    Type *cell = new_type(TYPE_UNKNOWN);
    cell->is_structure_cell = 1;
    return cell;
}

/* células compatíveis: mesmo tipo, ou uma delas ainda sem tipo */
int cells_compatible(Type *left_type, Type *right_type)
{
    if (left_type->kind == TYPE_UNKNOWN || right_type->kind == TYPE_UNKNOWN)
    {
        return 1;
    }

    return left_type->kind == right_type->kind &&
           (left_type->kind != TYPE_TUPLE || same_tuple(left_type, right_type));
}

/* mesma versão da função serve para esses dois tipos de argumento? */
int same_signature(Type *left_type, Type *right_type)
{
    if (left_type->kind != right_type->kind)
    {
        return 0;
    }

    switch (left_type->kind)
    {
    case TYPE_ARRAY:
        return left_type->dimension_count == right_type->dimension_count &&
               cells_compatible(left_type->element, right_type->element);

    case TYPE_TUPLE:
        return same_tuple(left_type, right_type);

    case TYPE_HEAP:
        return left_type->descending == right_type->descending &&
               cells_compatible(left_type->element, right_type->element);

    case TYPE_QUEUE:
        return cells_compatible(left_type->element, right_type->element);

    case TYPE_MAP:
        return cells_compatible(left_type->element, right_type->element) &&
               cells_compatible(left_type->value, right_type->value);

    case TYPE_GRAPH:
        return left_type->directed == right_type->directed &&
               cells_compatible(left_type->element, right_type->element);

    case TYPE_FENWICK:
        return cells_compatible(left_type->element, right_type->element);

    case TYPE_SEGTREE:
    case TYPE_RMQ:
        return left_type->combine == right_type->combine &&
               cells_compatible(left_type->element, right_type->element);

    default:
        return 1;
    }
}

/* valor que sai de uma estrutura (heap, fila, vetor): sem tipo ainda vira inteiro */
Type *taken_value(Type *cell)
{
    return resolve_unknown(cell);
}
