#include "sema_internal.h"

/** Checa a expressão e exige que seja inteira. */
Type *check_int(Expression *expr)
{
    Type *type = resolve_unknown(check_expression(expr));

    if (type->kind != TYPE_INT && type->kind != TYPE_CHAR)
    {
        FAIL_AT_NODE(expr, "esperava inteiro, mas é %s", type_name(type));
    }

    return type;
}

/** Checa a expressão e exige que seja número. */
Type *check_number(Expression *expr)
{
    Type *type = resolve_unknown(check_expression(expr));

    if (!is_number(type))
    {
        FAIL_AT_NODE(expr, "esperava número, mas é %s", type_name(type));
    }

    return type;
}

/** Checa a expressão e exige que seja vetor. */
Type *check_array(Expression *expr)
{
    Type *type = check_expression(expr);

    if (type->kind != TYPE_ARRAY)
    {
        FAIL_AT_NODE(expr, "esperava vetor, mas é %s", type_name(type));
    }

    return type;
}

/* gerador: soma(corpo para i=0 ate n) / soma(corpo para x em v) */
Type *check_generator(Expression *generator)
{
    Scope scope = {.parent = current_scope};
    current_scope = &scope;
    Type *value_type;

    if (generator->second)
    {
        check_int(generator->second);
        check_int(generator->third);
        value_type = &INT_TYPE;
    }
    else
    {
        Type *iterable_type = check_array(generator->third);
        value_type = iterable_type->dimension_count == 1
                         ? owned_type(iterable_type->element)
                         : array_type(iterable_type->element, iterable_type->dimension_count - 1);
    }

    generator->variable = declare_in_scope(&scope, generator->text, value_type, generator->line);
    Type *type = check_number(generator->first);
    current_scope = scope.parent;
    return generator->type = type;
}

/* soma / min / max */
Type *check_reduce(Expression *expr, Builtin builtin)
{
    const char *c_name = expr->text;

    if (expr->arguments.count == 0)
    {
        FAIL_AT_NODE(expr, "%s precisa de argumento", c_name);
    }

    if (expr->arguments.count == 1)
    {
        Expression *argument = expr->arguments.items[0];

        if (argument->kind == EXPR_GENERATOR)
        {
            return check_generator(argument);
        }

        Type *type = check_array(argument);

        if (type->dimension_count != 1)
        {
            FAIL_AT_NODE(argument, "%s funciona com vetor 1D (ou use 'para' dentro)", c_name);
        }

        return value_of_cell(resolve_unknown(type->element));
    }

    if (builtin == BUILTIN_SOMA)
    {
        FAIL_AT_NODE(expr, "soma recebe um vetor ou um gerador (soma(x para i=0 ate n))");
    }

    int has_real = 0;

    for (int i = 0; i < expr->arguments.count; i++)
    {
        has_real |= check_number(expr->arguments.items[i])->kind == TYPE_REAL;
    }

    return has_real ? &REAL_TYPE : &INT_TYPE;
}

/** Checa uma chamada de função do usuário, criando a versão para esses tipos se preciso. */
Type *check_user_call(Expression *expr, Function *function)
{
    Statement *definition = function->definition;

    if (expr->arguments.count != definition->parameters.count)
    {
        FAIL_AT_NODE(expr, "%s recebe %d argumento(s), mas recebeu %d", function->name,
                     definition->parameters.count, expr->arguments.count);
    }

    Type **argument_types = allocate_zeroed(sizeof(Type *) * (size_t)(expr->arguments.count + 1));

    for (int i = 0; i < expr->arguments.count; i++)
    {
        Expression *argument = expr->arguments.items[i];

        if (argument->kind == EXPR_GENERATOR)
        {
            FAIL_AT_NODE(argument, "gerador só pode ser usado em soma/min/max");
        }

        Type *type = check_expression(argument);

        if (type->kind == TYPE_STRING || type->kind == TYPE_VOID)
        {
            FAIL_AT_NODE(argument, "não dá para passar %s para uma função", type_name(type));
        }

        argument_types[i] = owned_type(type);

        /* parâmetro com tipo escrito: o argumento precisa combinar (inteiro vira real) */
        TypeSyntax *declared = definition->parameter_types ? definition->parameter_types[i] : NULL;

        if (declared)
        {
            Type *wanted = type_from_syntax(declared, 0);

            unify_types(wanted, type, argument);
            argument_types[i] = is_number(wanted) ? owned_type(wanted) : type;
        }
    }

    FunctionInstance *instance = find_instance(function, argument_types, expr->arguments.count);

    if (!instance)
    {
        expr->instance = new_instance(function, argument_types);
        return expr->instance->return_type;
    }

    for (int i = 0; i < expr->arguments.count; i++)
    {
        if (argument_types[i]->kind >= TYPE_ARRAY && argument_types[i]->kind != TYPE_TUPLE &&
            argument_types[i]->kind != TYPE_DSU)
        {
            unify_types(instance->parameter_types[i], argument_types[i], expr->arguments.items[i]);
        }
    }

    if (instance->is_being_checked && !instance->return_type)
    {
        FAIL_AT_NODE(
            expr,
            "não consigo descobrir o que '%s' retorna: coloque um 'ret' com valor antes da "
            "chamada recursiva",
            function->name);
    }

    expr->instance = instance;
    return instance->return_type;
}

/** Exige entre lo e hi argumentos na chamada. */
void check_argument_count(Expression *expr, int lo, int hi)
{
    if (expr->arguments.count < lo || expr->arguments.count > hi)
    {
        if (lo == hi)
        {
            FAIL_AT_NODE(expr, "'%s' recebe %d argumento(s), mas recebeu %d", expr->text, lo,
                         expr->arguments.count);
        }

        FAIL_AT_NODE(expr, "'%s' recebe de %d a %d argumentos, mas recebeu %d", expr->text, lo, hi,
                     expr->arguments.count);
    }
}

/* palavra da linguagem usada como argumento: heap(maior), grafo(n, dirigido) */
int is_word(Expression *argument, Builtin builtin)
{
    return argument->kind == EXPR_NAME &&
           builtin_of(argument->text, checked_program->language) == builtin &&
           !lookup_variable(argument->text);
}

/** Tipo de uma operação binária (aritmética, comparação, lógica, em). */
Type *check_binary(Expression *expr)
{
    if (expr->operation == KEYWORD_EM)
    {
        Type *map_type = check_expression(expr->second);

        if (map_type->kind != TYPE_MAP)
        {
            FAIL_AT_NODE(expr->second, "'em' por enquanto funciona com mapa, e isto é %s",
                         type_name(map_type));
        }

        store_in_cell(map_type->element, check_expression(expr->first), expr->first);
        return &INT_TYPE;
    }

    if (expr->operation >= PUNCT_EQUAL && expr->operation <= PUNCT_GREATER_EQUAL)
    {
        Type *left_type = resolve_unknown(check_expression(expr->first));
        Type *right_type = resolve_unknown(check_expression(expr->second));

        if (is_string(left_type) || is_string(right_type))
        {
            if (!is_string(left_type) || !is_string(right_type))
            {
                FAIL_AT_NODE(expr, "só dá para comparar texto com texto");
            }

            return &INT_TYPE;
        }

        if (left_type->kind == TYPE_TUPLE || right_type->kind == TYPE_TUPLE)
        {
            if (left_type->kind != TYPE_TUPLE || right_type->kind != TYPE_TUPLE ||
                !same_tuple(left_type, right_type))
            {
                FAIL_AT_NODE(expr, "só dá para comparar tuplas do mesmo formato");
            }

            return &INT_TYPE;
        }

        if (!is_number(left_type) || !is_number(right_type))
        {
            FAIL_AT_NODE(expr, "só dá para comparar números ou tuplas");
        }

        return &INT_TYPE;
    }

    if (expr->operation == KEYWORD_E || expr->operation == KEYWORD_OU)
    {
        check_number(expr->first);
        check_number(expr->second);
        return &INT_TYPE;
    }

    if (expr->operation == PUNCT_PERCENT || expr->operation == PUNCT_AMPERSAND ||
        expr->operation == PUNCT_BAR || expr->operation == PUNCT_CARET ||
        expr->operation == PUNCT_SHIFT_LEFT || expr->operation == PUNCT_SHIFT_RIGHT)
    {
        check_int(expr->first);
        check_int(expr->second);
        return &INT_TYPE;
    }

    /* texto + texto e texto + caractere: uma string nova */
    if (expr->operation == PUNCT_PLUS)
    {
        Type *left_value = resolve_unknown(check_expression(expr->first));

        if (is_string(left_value))
        {
            Type *right_value = resolve_unknown(check_expression(expr->second));

            if (!is_string(right_value) && right_value->kind != TYPE_CHAR &&
                right_value->kind != TYPE_INT)
            {
                FAIL_AT_NODE(expr->second, "texto + %s não existe (use str(x) para números)",
                             type_name(right_value));
            }

            return new_string_type();
        }
    }

    Type *left_type = check_number(expr->first);
    Type *right_type = check_number(expr->second);
    return (left_type->kind == TYPE_REAL || right_type->kind == TYPE_REAL) ? &REAL_TYPE : &INT_TYPE;
}

/** Checa um método de vetor, heap, fila, mapa ou grafo e devolve o tipo do resultado. */
/** segtree(v, min|max|soma) e rmq(v, min|max): estrutura de consulta de intervalo sobre v. */
Type *check_range_structure(Expression *expr, Builtin builtin)
{
    check_argument_count(expr, 2, 2);
    Type *source = check_array(expr->arguments.items[0]);
    Expression *operation = expr->arguments.items[1];
    int is_rmq = builtin == BUILTIN_RMQ;

    if (source->dimension_count != 1 || !is_number(resolve_unknown(source->element)))
    {
        FAIL_AT_NODE(expr->arguments.items[0], "'%s' recebe um vetor 1D de números", expr->text);
    }

    Type *type = new_type(is_rmq ? TYPE_RMQ : TYPE_SEGTREE);
    type->element = new_structure_cell();
    type->element->kind = source->element->kind;

    if (is_word(operation, BUILTIN_MIN))
    {
        type->combine = COMBINE_MIN;
    }
    else if (is_word(operation, BUILTIN_MAX))
    {
        type->combine = COMBINE_MAX;
    }
    else if (!is_rmq && is_word(operation, BUILTIN_SOMA))
    {
        type->combine = COMBINE_SUM;
    }
    else
    {
        FAIL_AT_NODE(operation, is_rmq ? "rmq combina com min ou max"
                                       : "segtree combina com min, max ou soma");
    }

    return type;
}

Type *check_method(Expression *expr)
{
    Type *type = check_expression(expr->first);

    if (type->kind == TYPE_UNKNOWN && expr->first->kind == EXPR_NAME)
    {
        type->kind = TYPE_ARRAY;
        type->element = new_type(TYPE_UNKNOWN);
        type->dimension_count = 1;
    }

    Method method = method_of(type->kind, expr->text, checked_program->language);

    if (type->kind == TYPE_ARRAY && type->dimension_count != 1 && method != METHOD_NONE)
    {
        FAIL_AT_NODE(expr, "'%s' só existe em vetor 1D", expr->text);
    }

    switch (method)
    {
    case METHOD_ADICIONAR:
    case METHOD_INSERIR:
    case METHOD_ENFILEIRAR:
    {
        check_argument_count(expr, 1, 1);
        Type *value = resolve_unknown(check_expression(expr->arguments.items[0]));

        /* texto.add(outro texto) acrescenta no fim */
        if (is_string(type) && is_string(value))
        {
            return &VOID_TYPE;
        }

        store_in_cell(type->element, value, expr->arguments.items[0]);
        return &VOID_TYPE;
    }

    case METHOD_REMOVER:
        if (type->kind == TYPE_MAP)
        {
            check_argument_count(expr, 1, 1);
            store_in_cell(type->element, check_expression(expr->arguments.items[0]),
                          expr->arguments.items[0]);
            return &VOID_TYPE;
        }

        check_argument_count(expr, 0, 0);
        return taken_value(type->element);

    case METHOD_REMOVER_ULTIMO:
    case METHOD_TOPO:
    case METHOD_DESENFILEIRAR:
    case METHOD_FRENTE:
        check_argument_count(expr, 0, 0);
        return taken_value(type->element);

    case METHOD_ARESTA:
        check_argument_count(expr, 2, 3);
        check_int(expr->arguments.items[0]);
        check_int(expr->arguments.items[1]);

        if (expr->arguments.count == 3)
        {
            type->weighted = 1;
            store_in_cell(type->element, check_number(expr->arguments.items[2]),
                          expr->arguments.items[2]);
        }

        return &VOID_TYPE;

    case METHOD_UNIR:
    case METHOD_JUNTOS:
        check_argument_count(expr, 2, 2);
        check_int(expr->arguments.items[0]);
        check_int(expr->arguments.items[1]);
        return &INT_TYPE;

    case METHOD_ACHAR:
    case METHOD_TAM_GRUPO:
        check_argument_count(expr, 1, 1);
        check_int(expr->arguments.items[0]);
        return &INT_TYPE;

    case METHOD_GRUPOS:
        check_argument_count(expr, 0, 0);
        return &INT_TYPE;

    case METHOD_SOMAR:
    case METHOD_MUDAR:
        check_argument_count(expr, 2, 2);
        check_int(expr->arguments.items[0]);
        store_in_cell(type->element, check_number(expr->arguments.items[1]),
                      expr->arguments.items[1]);
        return &VOID_TYPE;

    case METHOD_SOMA:
    case METHOD_CONSULTA:
        check_argument_count(expr, 2, 2);
        check_int(expr->arguments.items[0]);
        check_int(expr->arguments.items[1]);
        return taken_value(type->element);

    case METHOD_VIZINHOS:
        FAIL_AT_NODE(expr, "vizinhos() se usa num laço: 'para v em g.vizinhos(u)'");

    default:
        FAIL_AT_NODE(expr, "%s não tem o método '%s'", type_name(type), expr->text);
    }
}

/** Checa uma expressão, anota o tipo dela e o devolve. */
Type *check_expression(Expression *expr)
{
    switch (expr->kind)
    {
    case EXPR_INT:
        return expr->type = &INT_TYPE;

    case EXPR_REAL:
        return expr->type = &REAL_TYPE;

    case EXPR_STRING:
        return expr->type = new_string_type();

    case EXPR_CHAR:
        return expr->type = &CHAR_TYPE;

    case EXPR_NAME:
    {
        Variable *variable = lookup_variable(expr->text);

        if (!variable && builtin_of(expr->text, checked_program->language) == BUILTIN_INF)
        {
            /* INF vira um número; o int32 guarda INF de um jeito especial (gen.c) */
            expr->kind = EXPR_INT;
            expr->int_value = ALG_INF;
            checked_program->uses_inf = 1;
            return expr->type = &INT_TYPE;
        }

        if (!variable)
        {
            FAIL_AT_NODE(expr,
                         "'%s' não foi declarada (atribua um valor, leia com ler() ou use #decl)",
                         expr->text);
        }

        expr->variable = variable;
        return expr->type = variable->type;
    }

    case EXPR_TUPLE:
    {
        Type *type = new_type(TYPE_TUPLE);

        type->field_count = expr->arguments.count;
        type->fields = allocate_zeroed(sizeof(Type *) * (size_t)expr->arguments.count);

        for (int i = 0; i < expr->arguments.count; i++)
        {
            Expression *field = expr->arguments.items[i];

            /* string num campo fica guardada compacta (Str), como nas estruturas */
            Type *field_type = resolve_unknown(check_expression(field));

            /* tupla dentro de tupla é guardada por valor, como os outros campos */
            type->fields[i] = is_string(field_type)             ? new_type(TYPE_STRING)
                              : field_type->kind == TYPE_TUPLE ? owned_type(field_type)
                                                               : owned_type(check_number(field));
        }

        return expr->type = type;
    }

    case EXPR_MAP:
    {
        Type *type = new_type(TYPE_MAP);

        type->element = new_structure_cell();
        type->value = new_structure_cell();
        return expr->type = type;
    }

    case EXPR_BINARY:
        return expr->type = check_binary(expr);

    case EXPR_UNARY:
        if (expr->operation == PUNCT_TILDE)
        {
            return expr->type = check_int(expr->first);
        }

        if (expr->operation == KEYWORD_NAO)
        {
            check_number(expr->first);
            return expr->type = &INT_TYPE;
        }

        return expr->type = check_number(expr->first);

    case EXPR_INDEX:
    {
        Type *type = check_expression(expr->first);

        if (type->kind == TYPE_MAP)
        {
            store_in_cell(type->element, check_expression(expr->second), expr->second);
            return expr->type = value_of_cell(type->value);
        }

        /* p[0], p[1]: o índice é um número fixo, porque cada campo pode ter um tipo */
        if (type->kind == TYPE_TUPLE)
        {
            if (expr->second->kind != EXPR_INT || expr->second->int_value < 0 ||
                expr->second->int_value >= type->field_count)
            {
                FAIL_AT_NODE(expr->second, "campo de tupla é um número fixo, de 0 a %d",
                             type->field_count - 1);
            }

            expr->second->type = &INT_TYPE;
            return expr->type = value_of_cell(type->fields[expr->second->int_value]);
        }

        if (type->kind != TYPE_ARRAY)
        {
            FAIL_AT_NODE(expr, "só dá para indexar vetor, mapa ou tupla, e isto é %s",
                         type_name(type));
        }

        check_int(expr->second);
        return expr->type = type->dimension_count == 1
                                ? value_of_cell(type->element)
                                : array_type(type->element, type->dimension_count - 1);
    }

    case EXPR_SLICE:
    {
        Type *type = check_expression(expr->first);

        if (type->kind != TYPE_ARRAY)
        {
            FAIL_AT_NODE(expr, "só dá para fatiar vetor, e isto é %s", type_name(type));
        }

        if (expr->first->kind == EXPR_SLICE)
        {
            FAIL_AT_NODE(expr, "fatia de fatia em várias dimensões só é aceita dentro de ler()");
        }

        check_int(expr->second);
        check_int(expr->third);
        return expr->type = array_type(type->element, type->dimension_count);
    }

    case EXPR_VECTOR:
    {
        Type *cell = new_type(TYPE_UNKNOWN);

        for (int i = 0; i < expr->arguments.count; i++)
        {
            Type *type = resolve_unknown(check_expression(expr->arguments.items[i]));

            if (cell->kind == TYPE_INT && type->kind == TYPE_REAL)
            {
                cell->kind = TYPE_REAL;
            }
            else if (!(cell->kind == TYPE_REAL && type->kind == TYPE_INT))
            {
                store_in_cell(cell, type, expr->arguments.items[i]);
            }
        }

        return expr->type = array_type(cell, 1);
    }

    case EXPR_GENERATOR:
        FAIL_AT_NODE(expr,
                     "gerador ('para' dentro de parênteses) só pode ser usado em soma/min/max");

    case EXPR_METHOD:
        return expr->type = check_method(expr);

    case EXPR_CALL:
    {
        Builtin builtin = builtin_of(expr->text, checked_program->language);

        switch (builtin)
        {
        case BUILTIN_LER:
        case BUILTIN_ESCREVER:
            FAIL_AT_NODE(expr, "%s é um comando, não devolve valor", expr->text);

        case BUILTIN_TAMANHO:
        {
            check_argument_count(expr, 1, 1);
            Type *type = check_expression(expr->arguments.items[0]);

            if (type->kind == TYPE_STRING || type->kind == TYPE_VOID || type->kind == TYPE_INT ||
                type->kind == TYPE_REAL || type->kind == TYPE_TUPLE || type->kind == TYPE_UNKNOWN)
            {
                FAIL_AT_NODE(expr->arguments.items[0], "%s() não funciona com %s", expr->text,
                             type_name(type));
            }

            return expr->type = &INT_TYPE;
        }

        case BUILTIN_COPIA:
        {
            check_argument_count(expr, 1, 1);
            Type *type = check_array(expr->arguments.items[0]);
            return expr->type = array_type(type->element, type->dimension_count);
        }

        case BUILTIN_SOMA:
        case BUILTIN_MIN:
        case BUILTIN_MAX:
            return expr->type = check_reduce(expr, builtin);

        case BUILTIN_INT:
        case BUILTIN_REAL:
        case BUILTIN_INT32:
            check_argument_count(expr, 1, 1);
            check_number(expr->arguments.items[0]);
            return expr->type = builtin == BUILTIN_REAL ? &REAL_TYPE : &INT_TYPE;

        case BUILTIN_ORDENAR:
        {
            check_argument_count(expr, 1, 2);
            Type *type = check_array(expr->arguments.items[0]);

            if (type->dimension_count != 1)
            {
                FAIL_AT_NODE(expr->arguments.items[0],
                             "ordenar() funciona com vetor 1D (ou uma linha: ordenar(m[i]))");
            }

            resolve_unknown(type->element);

            if (expr->arguments.count == 2 && !is_word(expr->arguments.items[1], BUILTIN_MAIOR))
            {
                FAIL_AT_NODE(expr->arguments.items[1],
                             "o segundo argumento de ordenar é 'maior' (decrescente)");
            }

            return expr->type = &VOID_TYPE;
        }

        case BUILTIN_HEAP:
        {
            check_argument_count(expr, 0, 1);
            Type *type = new_type(TYPE_HEAP);

            if (expr->arguments.count == 1 && !is_word(expr->arguments.items[0], BUILTIN_MAIOR))
            {
                FAIL_AT_NODE(expr->arguments.items[0],
                             "heap() aceita só 'maior' (o maior sai primeiro)");
            }

            type->element = new_structure_cell();
            type->descending = expr->arguments.count == 1;
            return expr->type = type;
        }

        case BUILTIN_FILA:
        {
            check_argument_count(expr, 0, 0);
            Type *type = new_type(TYPE_QUEUE);
            type->element = new_structure_cell();
            return expr->type = type;
        }

        case BUILTIN_GRAFO:
        {
            check_argument_count(expr, 1, 2);
            check_int(expr->arguments.items[0]);

            if (expr->arguments.count == 2 && !is_word(expr->arguments.items[1], BUILTIN_DIRIGIDO))
            {
                FAIL_AT_NODE(expr->arguments.items[1], "o segundo argumento de grafo é 'dirigido'");
            }

            Type *type = new_type(TYPE_GRAPH);
            type->element = new_structure_cell();
            type->directed = expr->arguments.count == 2;
            return expr->type = type;
        }

        case BUILTIN_TEXTO:
            check_argument_count(expr, 1, 1);
            check_number(expr->arguments.items[0]);
            return expr->type = new_string_type();

        case BUILTIN_LER_LINHA:
            FAIL_AT_NODE(expr, "%s é um comando, não devolve valor", expr->text);

        case BUILTIN_PREFIXO:
        {
            check_argument_count(expr, 1, 1);
            Type *source = check_array(expr->arguments.items[0]);

            if (source->dimension_count > 2 || !is_number(resolve_unknown(source->element)))
            {
                FAIL_AT_NODE(expr->arguments.items[0],
                             "prefixo() recebe vetor ou matriz de números");
            }

            Type *cell = new_structure_cell();
            cell->kind = source->element->kind;
            return expr->type = array_type(cell, source->dimension_count);
        }

        case BUILTIN_BUSCA:
        {
            check_argument_count(expr, 2, 3);
            Type *source = check_array(expr->arguments.items[0]);

            if (source->dimension_count != 1)
            {
                FAIL_AT_NODE(expr->arguments.items[0], "busca() funciona com vetor 1D ordenado");
            }

            Type *wanted = resolve_unknown(check_expression(expr->arguments.items[1]));
            Type *element = resolve_unknown(source->element);
            int comparable = (is_number(wanted) && is_number(element)) ||
                             (wanted->kind == TYPE_TUPLE && element->kind == TYPE_TUPLE &&
                              same_tuple(wanted, element));

            if (!comparable)
            {
                FAIL_AT_NODE(expr->arguments.items[1],
                             "busca() procura um valor do mesmo tipo dos elementos");
            }

            if (expr->arguments.count == 3 && !is_word(expr->arguments.items[2], BUILTIN_MAIOR))
            {
                FAIL_AT_NODE(expr->arguments.items[2], "o terceiro argumento de busca é 'maior'");
            }

            return expr->type = &INT_TYPE;
        }

        case BUILTIN_MDC:
        case BUILTIN_MMC:
            check_argument_count(expr, 2, 2);
            check_int(expr->arguments.items[0]);
            check_int(expr->arguments.items[1]);
            return expr->type = &INT_TYPE;

        case BUILTIN_POTMOD:
            check_argument_count(expr, 3, 3);

            for (int i = 0; i < 3; i++)
            {
                check_int(expr->arguments.items[i]);
            }

            return expr->type = &INT_TYPE;

        case BUILTIN_CRIVO:
        {
            check_argument_count(expr, 1, 1);
            check_int(expr->arguments.items[0]);

            Type *cell = new_structure_cell();
            cell->kind = TYPE_INT;
            return expr->type = array_type(cell, 1);
        }

        case BUILTIN_CONJUNTOS:
            check_argument_count(expr, 1, 1);
            check_int(expr->arguments.items[0]);
            return expr->type = new_type(TYPE_DSU);

        case BUILTIN_FENWICK:
        {
            check_argument_count(expr, 1, 1);
            check_int(expr->arguments.items[0]);

            Type *type = new_type(TYPE_FENWICK);
            type->element = new_structure_cell();
            return expr->type = type;
        }

        case BUILTIN_SEGTREE:
        case BUILTIN_RMQ:
            return expr->type = check_range_structure(expr, builtin);

        case BUILTIN_MAPA:
            FAIL_AT_NODE(expr, "mapa se cria com 'mapa {}'");

        case BUILTIN_MAIOR:
        case BUILTIN_DIRIGIDO:
        case BUILTIN_INF:
            FAIL_AT_NODE(expr, "'%s' não é função", expr->text);

        case BUILTIN_ADICIONAR:
        case BUILTIN_REMOVER_ULTIMO:
            FAIL_AT_NODE(expr, "%s é método: use v.%s(...)", expr->text, expr->text);

        default:
            break;
        }

        Function *function = find_function(expr->text);

        if (!function)
        {
            FAIL_AT_NODE(expr, "a função '%s' não existe", expr->text);
        }

        return expr->type = check_user_call(expr, function);
    }
    }

    return NULL;
}
