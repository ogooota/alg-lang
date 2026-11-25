#include "gen_internal.h"

/** Escreve o acesso a uma variável (com a criação no primeiro uso, se for #decl). */
void gen_variable(TextBuffer *out, Variable *variable, int line)
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
void gen_array_pointer(TextBuffer *out, Expression *expr)
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

const char *c_operator(TokenKind operation)
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
void gen_call2(TextBuffer *out, const char *function_suffix, Expression *item, Expression *right)
{
    text_appendf(out, "%s(", function_suffix);
    gen_expression(out, item);
    text_append(out, ", ");
    gen_expression(out, right);
    text_append(out, ")");
}

/* comparação de tuplas: ordem lexicográfica (campo a campo) */
void gen_tuple_comparison(TextBuffer *out, Expression *expr)
{
    const char *prefix =
        is_text_type(expr->first->type) ? "str" : tuple_element_kind(expr->first->type)->suffix;
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
void gen_binary(TextBuffer *out, Expression *expr)
{
    /* conjuntos: union 0, inter 1, minus 2 (map_*_combine em rt/map.h) */
    if (expr->operation == KEYWORD_UNIAO || expr->operation == KEYWORD_INTER ||
        expr->operation == KEYWORD_MENOS)
    {
        text_appendf(out, "%s_combine(", template_name(expr->type));
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_appendf(out, ", %d)", expr->operation == KEYWORD_UNIAO   ? 0
                                   : expr->operation == KEYWORD_INTER ? 1
                                                                      : 2);
        return;
    }

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
        (expr->first->type->kind == TYPE_TUPLE || is_text_type(expr->first->type)))
    {
        gen_tuple_comparison(out, expr);
        return;
    }

    /* texto + texto ou texto + caractere */
    if (expr->operation == PUNCT_PLUS && is_text_type(expr->first->type))
    {
        text_append(out, is_text_type(expr->second->type) ? "str_concat(" : "str_concat_char(");
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_append(out, ")");
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
void gen_generator_open(TextBuffer *out, Expression *generator, int temp_id)
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
void gen_reduce(TextBuffer *out, Expression *expr, Builtin builtin)
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
void gen_loaded(TextBuffer *out, Type *array, const char *element)
{
    text_appendf(out, "%s(%s)", cell_element_kind(array->element)->loader, element);
}

/** prefixo(v): P[i] = soma de v[0..i); em matriz, o prefixo 2D. */
void gen_prefix(TextBuffer *out, Expression *expr)
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
void gen_search(TextBuffer *out, Expression *expr)
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
void gen_range_structure(TextBuffer *out, Expression *expr)
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

void gen_call(TextBuffer *out, Expression *expr)
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

        case BUILTIN_TEXTO:
            text_append(out, expr->arguments.items[0]->type->kind == TYPE_REAL ? "str_from_real("
                                                                               : "str_from_int(");
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, ")");
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

        case BUILTIN_CONTA_PRIMOS:
            text_append(out, "count_primes(");
            gen_expression(out, expr->arguments.items[0]);
            text_append(out, ")");
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

/* m[k] op= x: a leitura de m[k] usa o ponteiro já buscado (_p<id>) em vez de buscar de novo */
Expression *cached_map_index;
int cached_map_pointer;

/* posição de vetor como lugar onde se escreve (sem a conversão de leitura) */
void gen_lvalue(TextBuffer *out, Expression *expr)
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

    /* p[0] = x: o campo do lugar onde a tupla está guardada (variável, vetor, mapa) */
    if (expr->kind == EXPR_INDEX && expr->first->type->kind == TYPE_TUPLE)
    {
        text_append(out, "(");
        gen_lvalue(out, expr->first);
        text_appendf(out, ").field_%lld", (long long)expr->second->int_value);
        return;
    }

    if (expr->kind != EXPR_INDEX || expr->first->type->dimension_count != 1)
    {
        gen_expression(out, expr);
        return;
    }

    /* s[i] = c numa variável string: se ela veio de uma estrutura, copia antes de mudar */
    if (expr->first->kind == EXPR_NAME && is_text_type(expr->first->type))
    {
        text_append(out, "(*str_at_mut(&");
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->second);
        text_appendf(out, ", %d))", expr->line);
        return;
    }

    text_appendf(out, "(*array_%s_at(", array_suffix(expr->first->type));
    gen_expression(out, expr->first);
    text_append(out, ", ");
    gen_expression(out, expr->second);
    text_appendf(out, ", %d))", expr->line);
}

/* valor que vai ser guardado numa célula: com ': int32', confere se cabe */
void gen_stored_value(TextBuffer *out, Type *cell, Expression *value)
{
    /* string guardada em estrutura é copiada: mudar a original depois não muda a guardada */
    if (is_text_type(value->type) && (cell_root(cell)->kind == TYPE_STRING || is_text_type(cell)))
    {
        text_append(out, "str_pack(");
        gen_expression(out, value);
        text_append(out, ")");
        return;
    }

    if (!is_checked_int32(cell))
    {
        gen_expression(out, value);
        return;
    }

    text_append(out, generated_program->uses_inf ? "encode_int32_inf(" : "fit_int32(");
    gen_expression(out, value);
    text_appendf(out, ", %d)", value->line);
}

/** Escreve um método de vetor ou de estrutura. */
void gen_method_call(TextBuffer *out, Expression *expr)
{
    Type *type = expr->first->type;
    Method method = method_of(type->kind, expr->text, generated_program->language);

    if (type->kind == TYPE_ARRAY && method == METHOD_ADICIONAR && is_text_type(type) &&
        is_text_type(expr->arguments.items[0]->type))
    {
        text_append(out, "str_append(");
        gen_array_pointer(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->arguments.items[0]);
        text_appendf(out, ", %d)", expr->line);
        return;
    }

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

    /* s.add(x) num conjunto: a chave passa a existir */
    if (type->kind == TYPE_MAP && method == METHOD_ADICIONAR)
    {
        text_appendf(out, "(*%s_ref(", template_name(type));
        gen_expression(out, expr->first);
        text_append(out, ", ");
        gen_expression(out, expr->arguments.items[0]);
        text_append(out, ") = 1)");
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

    /* heap/fila de strings guarda Str: o que sai volta a ser string */
    int unpack = (method == METHOD_REMOVER || method == METHOD_DESENFILEIRAR ||
                  method == METHOD_TOPO || method == METHOD_FRENTE) &&
                 type->kind != TYPE_MAP && is_text_type(expr->type);

    text_append(out, unpack ? "str_unpack(" : "");
    text_appendf(out, "%s_%s(", c_name, function_suffix);
    gen_expression(out, expr->first);

    for (int i = 0; i < expr->arguments.count; i++)
    {
        Expression *argument = expr->arguments.items[i];
        int copy = is_text_type(argument->type) &&
                   (method == METHOD_INSERIR || method == METHOD_ENFILEIRAR);

        text_append(out, copy ? ", str_pack(" : ", ");
        gen_expression(out, argument);
        text_append(out, copy ? ")" : "");
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

    text_append(out, unpack ? "))" : ")");
}

/** Escreve um número real como literal C. */
void gen_real(TextBuffer *out, double number)
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
void gen_expression(TextBuffer *out, Expression *expr)
{
    switch (expr->kind)
    {
    case EXPR_INT:
        text_appendf(out, "INT64_C(%lld)", (long long)expr->int_value);
        return;

    case EXPR_REAL:
        gen_real(out, expr->real_value);
        return;

    case EXPR_CHAR:
        text_appendf(out, "INT64_C(%lld)", (long long)expr->int_value);
        return;

    case EXPR_STRING:
        text_append(out, "str_from(");
        emit_c_string(out, expr->text);
        text_appendf(out, ", %d)", (int)strlen(expr->text));
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
        if (expr == cached_map_index)
        {
            text_appendf(out, "(*_p%d)", cached_map_pointer);
        }
        else if (expr->first->type->kind == TYPE_TUPLE)
        {
            int unpack = is_text_type(expr->type);

            text_append(out, unpack ? "str_unpack(((" : "((");
            gen_expression(out, expr->first);
            text_appendf(out, ").field_%lld)", (long long)expr->second->int_value);
            text_append(out, unpack ? ")" : "");
        }
        else if (expr->first->type->kind == TYPE_MAP)
        {
            int unpack = is_text_type(expr->type);

            text_appendf(out, "%s%s_get(", unpack ? "str_unpack(" : "", template_name(expr->first->type));
            gen_expression(out, expr->first);
            text_append(out, ", ");
            gen_expression(out, expr->second);
            text_append(out, unpack ? "))" : ")");
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
            /* string no campo: cópia compacta, mudar a original depois não muda a tupla */
            int is_text = tuple_element_kind(expr->type)->field_tags[i] == 's';

            text_append(out, i ? ", " : "");
            text_append(out, is_text ? "str_pack(" : "");
            gen_expression(out, expr->arguments.items[i]);
            text_append(out, is_text ? ")" : "");
        }

        text_append(out, "})");
        return;

    case EXPR_MAP:
        if (!expr->arguments.count)
        {
            text_appendf(out, "%s_new()", template_name(expr->type));
            return;
        }

        /* set {a, b}: cria e põe cada chave (expressão do GNU C, como os outros literais) */
        {
            int temp_id = temp_counter++;
            const char *c_name = template_name(expr->type);

            text_appendf(out, "({ %s *_s%d = %s_new(); ", template_type(expr->type), temp_id, c_name);

            for (int i = 0; i < expr->arguments.count; i++)
            {
                text_appendf(out, "*%s_ref(_s%d, ", c_name, temp_id);
                gen_expression(out, expr->arguments.items[i]);
                text_append(out, ") = 1; ");
            }

            text_appendf(out, "_s%d; })", temp_id);
        }

        return;

    case EXPR_GENERATOR:
        return;
    }
}

/* ---------- ler / escrever ---------- */

void gen_read(TextBuffer *out, Expression *argument, int depth)
{
    indent(out, depth);

    /* string: ler(s) lê uma palavra (numa variável ou numa posição de vetor de strings) */
    if (argument->type && is_text_type(argument->type) &&
        (argument->kind == EXPR_NAME || argument->kind == EXPR_INDEX))
    {
        gen_lvalue(out, argument);
        text_appendf(out, " = %s(%d);\n", argument->kind == EXPR_INDEX ? "packed_read" : "str_read",
                     argument->line);
        return;
    }

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
void gen_write(TextBuffer *out, Expression *expr, int depth)
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

        if (argument->kind == EXPR_STRING)
        {
            /* literal: escreve direto, sem criar string */
            text_append(out, "write_text(");
            emit_c_string(out, argument->text);
            text_append(out, ");\n");
            continue;
        }

        if (is_text_type(argument->type))
        {
            text_append(out, "str_write(");
        }
        else if (argument->type->kind == TYPE_CHAR)
        {
            text_append(out, "write_char(");
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
