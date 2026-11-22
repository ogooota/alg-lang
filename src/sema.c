#include "alg.h"

typedef struct Scope
{
    VEC(Variable *) variables;
    struct Scope *parent;
} Scope;

static Program *program;
static VEC(Function *) functions;
static int loop_depth;

/* escopos: global, o da função atual e o atual (geradores abrem um escopo filho) */
static Scope global_scope;
static Scope *function_scope;
static Scope *current_scope;

/* instância de função sendo checada (NULL = programa principal) */
static FunctionInstance *current_instance;

/* #decl v[n]: as dimensões são checadas no fim do escopo, quando n já existe */
static VEC(Variable *) pending_dimensions;

static Type INT_TYPE = {.kind = TYPE_INT};
static Type REAL_TYPE = {.kind = TYPE_REAL};
static Type STRING_TYPE = {.kind = TYPE_STRING};
static Type VOID_TYPE = {.kind = TYPE_VOID};

/** Novo tipo do tipo básico pedido. */
static Type *new_type(TypeKind kind)
{
    Type *type = allocate_zeroed(sizeof *type);
    type->kind = kind;
    return type;
}

/** Tipo de vetor com o elemento e o número de dimensões dados. */
static Type *array_type(Type *elem, int nd)
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
static Type *resolve_unknown(Type *type)
{
    if (type->kind == TYPE_UNKNOWN)
    {
        type->kind = TYPE_INT;
    }

    return type;
}

/** Diz se o tipo é inteiro ou real. */
static int is_number(Type *type)
{
    return type->kind == TYPE_INT || type->kind == TYPE_REAL;
}

/** Nome legível de um tipo, para mensagens de erro. */
static const char *type_name(Type *type)
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
    }

    return "?";
}

/** Diz se duas tuplas têm o mesmo número de campos com os mesmos tipos. */
static int same_tuple(Type *left_type, Type *right_type)
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
static void fill_cell(Type *cell, Type *type)
{
    cell->kind = type->kind;
    cell->fields = type->fields;
    cell->field_count = type->field_count;
}

#define FAIL_AT_NODE(e, ...) fail_at((e)->line, (e)->column, __VA_ARGS__)

/* para variáveis novas: escalar vira tipo próprio (não compartilha célula); vetor compartilha */
static Type *owned_type(Type *type)
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
static void unify_elements(Type *target, Type *source, Expression *location)
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
    }
}

/* atribuir valor do tipo s num lugar do tipo d (inteiro → real é permitido) */
static void unify_types(Type *target, Type *source, Expression *location)
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
static void store_in_cell(Type *cell, Type *type, Expression *location)
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
static Type *new_structure_cell(void)
{
    Type *cell = new_type(TYPE_UNKNOWN);
    cell->is_structure_cell = 1;
    return cell;
}

/** Procura um nome do escopo atual para fora; NULL se não existe. */
static Variable *lookup_variable(const char *name)
{
    for (Scope *scope = current_scope; scope; scope = scope->parent)
    {
        for (int i = scope->variables.count - 1; i >= 0; i--)
        {
            if (!strcmp(scope->variables.items[i]->name, name))
            {
                return scope->variables.items[i];
            }
        }
    }

    return NULL;
}

/*
 * Variável do 'para': dentro de função ela é sempre local. Assim um laço
 * dentro de dfs() nunca sobrescreve o 'i' do laço do programa principal.
 */
static Variable *lookup_loop_variable(const char *name)
{
    if (!current_instance)
    {
        return lookup_variable(name);
    }

    for (Scope *scope = current_scope; scope; scope = scope->parent)
    {
        for (int i = scope->variables.count - 1; i >= 0; i--)
        {
            if (!strcmp(scope->variables.items[i]->name, name))
            {
                return scope->variables.items[i];
            }
        }

        if (scope == function_scope)
        {
            break;
        }
    }

    return NULL;
}

/** Cria uma variável no escopo dado. */
static Variable *declare_in_scope(Scope *scope, const char *name, Type *ty, int line)
{
    Variable *variable = allocate_zeroed(sizeof *variable);
    variable->name = duplicate_string(name);
    variable->type = ty;
    variable->line = line;
    PUSH(scope->variables, variable);
    return variable;
}

/** Cria uma variável no escopo da função atual (ou global no programa principal). */
static Variable *declare_variable(const char *name, Type *ty, int line)
{
    Variable *variable = declare_in_scope(function_scope, name, ty, line);

    if (function_scope == &global_scope)
    {
        variable->is_global = 1;
        PUSH(program->globals, variable);
    }
    else
    {
        PUSH(current_instance->locals, variable);
    }

    return variable;
}

/** Procura uma função do usuário pelo nome; NULL se não existe. */
static Function *find_function(const char *name)
{
    for (int i = 0; i < functions.count; i++)
    {
        if (!strcmp(functions.items[i]->name, name))
        {
            return functions.items[i];
        }
    }

    return NULL;
}

/* ---------- clonagem: cada instância de função tem sua cópia anotada ---------- */

static Expression *clone_expression(Expression *expr);

/** Cópia profunda de uma lista de expressões. */
static void clone_expression_list(ExpressionList *destination, ExpressionList *source)
{
    *destination = (ExpressionList){0};

    for (int i = 0; i < source->count; i++)
    {
        PUSH(*destination, clone_expression(source->items[i]));
    }
}

/** Cópia profunda de uma expressão (cada versão de função anota a sua). */
static Expression *clone_expression(Expression *expr)
{
    if (!expr)
    {
        return NULL;
    }

    Expression *copy = allocate_zeroed(sizeof *copy);

    *copy = *expr;
    copy->first = clone_expression(expr->first);
    copy->second = clone_expression(expr->second);
    copy->third = clone_expression(expr->third);
    clone_expression_list(&copy->arguments, &expr->arguments);
    return copy;
}

static Statement *clone_statement(Statement *stmt);

/** Cópia profunda de uma lista de comandos. */
static void clone_statement_list(StatementList *destination, StatementList *source)
{
    *destination = (StatementList){0};

    for (int i = 0; i < source->count; i++)
    {
        PUSH(*destination, clone_statement(source->items[i]));
    }
}

/** Cópia profunda de um comando. */
static Statement *clone_statement(Statement *stmt)
{
    Statement *copy = allocate_zeroed(sizeof *copy);

    *copy = *stmt;
    copy->first = clone_expression(stmt->first);
    copy->second = clone_expression(stmt->second);
    copy->third = clone_expression(stmt->third);
    clone_statement_list(&copy->body, &stmt->body);
    clone_statement_list(&copy->else_body, &stmt->else_body);
    copy->declarations.items = NULL;
    copy->declarations.count = 0;
    copy->declarations.capacity = 0;

    for (int i = 0; i < stmt->declarations.count; i++)
    {
        Declaration declaration = stmt->declarations.items[i];
        clone_expression_list(&declaration.dimensions, &stmt->declarations.items[i].dimensions);
        PUSH(copy->declarations, declaration);
    }

    return copy;
}

/* ---------- expressões ---------- */

static Type *check_expression(Expression *expr);
static void check_statements(StatementList *statements, int top);

/** Checa a expressão e exige que seja inteira. */
static Type *check_int(Expression *expr)
{
    Type *type = resolve_unknown(check_expression(expr));

    if (type->kind != TYPE_INT)
    {
        FAIL_AT_NODE(expr, "esperava inteiro, mas é %s", type_name(type));
    }

    return type;
}

/** Checa a expressão e exige que seja número. */
static Type *check_number(Expression *expr)
{
    Type *type = resolve_unknown(check_expression(expr));

    if (!is_number(type))
    {
        FAIL_AT_NODE(expr, "esperava número, mas é %s", type_name(type));
    }

    return type;
}

/** Checa a expressão e exige que seja vetor. */
static Type *check_array(Expression *expr)
{
    Type *type = check_expression(expr);

    if (type->kind != TYPE_ARRAY)
    {
        FAIL_AT_NODE(expr, "esperava vetor, mas é %s", type_name(type));
    }

    return type;
}

/* gerador: soma(corpo para i=0 ate n) / soma(corpo para x em v) */
static Type *check_generator(Expression *generator)
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
static Type *check_reduce(Expression *expr, Builtin builtin)
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

        return resolve_unknown(type->element);
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

/* células compatíveis: mesmo tipo, ou uma delas ainda sem tipo */
static int cells_compatible(Type *left_type, Type *right_type)
{
    if (left_type->kind == TYPE_UNKNOWN || right_type->kind == TYPE_UNKNOWN)
    {
        return 1;
    }

    return left_type->kind == right_type->kind &&
           (left_type->kind != TYPE_TUPLE || same_tuple(left_type, right_type));
}

/* mesma versão da função serve para esses dois tipos de argumento? */
static int same_signature(Type *left_type, Type *right_type)
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

    default:
        return 1;
    }
}

/** Checa os tamanhos dos #decl v[n] do escopo que está terminando. */
static void check_pending_dimensions(int from)
{
    for (int i = from; i < pending_dimensions.count; i++)
    {
        for (int j = 0; j < pending_dimensions.items[i]->dimensions.count; j++)
        {
            check_int(pending_dimensions.items[i]->dimensions.items[j]);
        }
    }

    pending_dimensions.count = from;
}

/* procura uma versão já existente da função para esses tipos de argumento */
static FunctionInstance *find_instance(Function *function, Type **argument_types, int count)
{
    for (FunctionInstance *instance = function->instances; instance; instance = instance->next)
    {
        int same = 1;

        for (int i = 0; i < count && same; i++)
        {
            same = same_signature(instance->parameter_types[i], argument_types[i]);
        }

        if (same)
        {
            return instance;
        }
    }

    return NULL;
}

/* cria uma versão nova: copia o corpo e checa com os tipos dos argumentos */
static FunctionInstance *new_instance(Function *function, Type **argument_types)
{
    Statement *definition = function->definition;
    FunctionInstance *instance = allocate_zeroed(sizeof *instance);

    instance->function = function;
    instance->parameter_types = argument_types;
    instance->is_being_checked = 1;
    instance->next = function->instances;
    function->instances = instance;

    instance->c_name = allocate_zeroed(strlen(function->name) + 24);
    sprintf(instance->c_name, "%s_%d", function->name, function->instance_count);
    function->instance_count++;

    clone_statement_list(&instance->body, &definition->body);
    PUSH(program->instances, instance);

    /* salva o contexto de quem chamou */
    FunctionInstance *saved_inst = current_instance;
    Scope *saved_sc = current_scope;
    Scope *saved_fscope = function_scope;
    int saved_loopdepth = loop_depth;
    int saved_pending = pending_dimensions.count;

    Scope *new_scope = allocate_zeroed(sizeof *new_scope);
    new_scope->parent = &global_scope;
    current_instance = instance;
    current_scope = new_scope;
    function_scope = new_scope;
    loop_depth = 0;

    instance->parameters =
        allocate_zeroed(sizeof(Variable *) * (size_t)(definition->parameters.count + 1));

    for (int i = 0; i < definition->parameters.count; i++)
    {
        for (int j = 0; j < i; j++)
        {
            if (!strcmp(definition->parameters.items[i], definition->parameters.items[j]))
            {
                FAIL_AT_NODE(definition, "parâmetro '%s' repetido",
                             definition->parameters.items[i]);
            }
        }

        instance->parameters[i] = declare_in_scope(new_scope, definition->parameters.items[i],
                                                   argument_types[i], definition->line);
        instance->parameters[i]->is_parameter = 1;
    }

    check_statements(&instance->body, 1);
    check_pending_dimensions(saved_pending);

    current_instance = saved_inst;
    current_scope = saved_sc;
    function_scope = saved_fscope;
    loop_depth = saved_loopdepth;

    if (!instance->return_type)
    {
        instance->return_type = &VOID_TYPE;
    }

    instance->is_being_checked = 0;
    return instance;
}

/** Checa uma chamada de função do usuário, criando a versão para esses tipos se preciso. */
static Type *check_user_call(Expression *expr, Function *function)
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
    }

    FunctionInstance *instance = find_instance(function, argument_types, expr->arguments.count);

    if (!instance)
    {
        expr->instance = new_instance(function, argument_types);
        return expr->instance->return_type;
    }

    for (int i = 0; i < expr->arguments.count; i++)
    {
        if (argument_types[i]->kind == TYPE_ARRAY || argument_types[i]->kind == TYPE_HEAP ||
            argument_types[i]->kind == TYPE_QUEUE || argument_types[i]->kind == TYPE_MAP ||
            argument_types[i]->kind == TYPE_GRAPH)
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
static void check_argument_count(Expression *expr, int lo, int hi)
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
static int is_word(Expression *argument, Builtin builtin)
{
    return argument->kind == EXPR_NAME &&
           builtin_of(argument->text, program->language) == builtin &&
           !lookup_variable(argument->text);
}

/** Tipo de uma operação binária (aritmética, comparação, lógica, em). */
static Type *check_binary(Expression *expr)
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

    Type *left_type = check_number(expr->first);
    Type *right_type = check_number(expr->second);
    return (left_type->kind == TYPE_REAL || right_type->kind == TYPE_REAL) ? &REAL_TYPE : &INT_TYPE;
}

/* valor que sai de uma estrutura (heap, fila, vetor): sem tipo ainda vira inteiro */
static Type *taken_value(Type *cell)
{
    return resolve_unknown(cell);
}

/** Checa um método de vetor, heap, fila, mapa ou grafo e devolve o tipo do resultado. */
static Type *check_method(Expression *expr)
{
    Type *type = check_expression(expr->first);

    if (type->kind == TYPE_UNKNOWN && expr->first->kind == EXPR_NAME)
    {
        type->kind = TYPE_ARRAY;
        type->element = new_type(TYPE_UNKNOWN);
        type->dimension_count = 1;
    }

    Method method = method_of(type->kind, expr->text, program->language);

    if (type->kind == TYPE_ARRAY && type->dimension_count != 1 && method != METHOD_NONE)
    {
        FAIL_AT_NODE(expr, "'%s' só existe em vetor 1D", expr->text);
    }

    switch (method)
    {
    case METHOD_ADICIONAR:
    case METHOD_INSERIR:
    case METHOD_ENFILEIRAR:
        check_argument_count(expr, 1, 1);
        store_in_cell(type->element, check_expression(expr->arguments.items[0]),
                      expr->arguments.items[0]);
        return &VOID_TYPE;

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

    case METHOD_VIZINHOS:
        FAIL_AT_NODE(expr, "vizinhos() se usa num laço: 'para v em g.vizinhos(u)'");

    default:
        FAIL_AT_NODE(expr, "%s não tem o método '%s'", type_name(type), expr->text);
    }
}

/** Checa uma expressão, anota o tipo dela e o devolve. */
static Type *check_expression(Expression *expr)
{
    switch (expr->kind)
    {
    case EXPR_INT:
        return expr->type = &INT_TYPE;

    case EXPR_REAL:
        return expr->type = &REAL_TYPE;

    case EXPR_STRING:
        return expr->type = &STRING_TYPE;

    case EXPR_NAME:
    {
        Variable *variable = lookup_variable(expr->text);

        if (!variable && builtin_of(expr->text, program->language) == BUILTIN_INF)
        {
            /* INF vira um número; o int32 guarda INF de um jeito especial (gen.c) */
            expr->kind = EXPR_INT;
            expr->int_value = ALG_INF;
            program->uses_inf = 1;
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
            type->fields[i] = owned_type(check_number(expr->arguments.items[i]));
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
            return expr->type = type->value;
        }

        if (type->kind != TYPE_ARRAY)
        {
            FAIL_AT_NODE(expr, "só dá para indexar vetor ou mapa, e isto é %s", type_name(type));
        }

        check_int(expr->second);
        return expr->type = type->dimension_count == 1
                                ? type->element
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
        Builtin builtin = builtin_of(expr->text, program->language);

        switch (builtin)
        {
        case BUILTIN_LER:
        case BUILTIN_ESCREVER:
            FAIL_AT_NODE(expr, "%s é um comando, não devolve valor", expr->text);

        case BUILTIN_TAMANHO:
        {
            check_argument_count(expr, 1, 1);
            Type *type = check_expression(expr->arguments.items[0]);

            if (type->kind != TYPE_ARRAY && type->kind != TYPE_HEAP && type->kind != TYPE_QUEUE &&
                type->kind != TYPE_MAP && type->kind != TYPE_GRAPH)
            {
                FAIL_AT_NODE(expr->arguments.items[0], "tam() não funciona com %s",
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

/* ---------- ler / escrever ---------- */

static void check_read_argument(Expression *argument)
{
    if (argument->kind == EXPR_NAME)
    {
        Variable *variable = lookup_variable(argument->text);

        if (!variable)
        {
            variable = declare_variable(argument->text, &INT_TYPE, argument->line);
        }

        argument->variable = variable;
        argument->type = variable->type;

        if (variable->type->kind == TYPE_UNKNOWN)
        {
            variable->type->kind = TYPE_INT;
        }

        if (variable->type->kind == TYPE_ARRAY)
        {
            resolve_unknown(variable->type->element);
            return;
        }

        if (!is_number(variable->type))
        {
            FAIL_AT_NODE(argument, "ler() não sabe ler %s", type_name(variable->type));
        }

        return;
    }

    if (argument->kind == EXPR_INDEX)
    {
        Type *type = resolve_unknown(check_expression(argument));

        if (!is_number(type))
        {
            FAIL_AT_NODE(argument, "ler() precisa chegar até os elementos (isto é %s)",
                         type_name(type));
        }

        return;
    }

    if (argument->kind != EXPR_SLICE)
    {
        FAIL_AT_NODE(argument, "ler() lê variáveis, posições ou fatias como a[0..n]");
    }

    int dimension_count = 0;
    Expression *base = argument;

    while (base->kind == EXPR_SLICE)
    {
        check_int(base->second);
        check_int(base->third);
        base = base->first;
        dimension_count++;
    }

    if (dimension_count > 4)
    {
        FAIL_AT_NODE(argument, "no máximo 4 dimensões");
    }

    Variable *variable = base->kind == EXPR_NAME ? lookup_variable(base->text) : NULL;

    if (base->kind == EXPR_NAME &&
        (!variable || (variable->type->kind != TYPE_ARRAY && variable->type != &INT_TYPE &&
                       variable->type != &REAL_TYPE && !variable->is_parameter)))
    {
        /* criação: ler(a[0..n]) cria a com n posições */
        if (variable && variable->type->kind != TYPE_UNKNOWN && !is_number(variable->type))
        {
            FAIL_AT_NODE(base, "'%s' já é %s", base->text, type_name(variable->type));
        }

        Type *cell = new_type(variable && variable->type->kind == TYPE_REAL ? TYPE_REAL : TYPE_INT);

        if (variable)
        {
            cell->is_int32 = variable->type->is_int32;
        }

        if (!variable)
        {
            variable = declare_variable(base->text, array_type(cell, dimension_count), base->line);
        }
        else
        {
            variable->type->kind = TYPE_ARRAY;
            variable->type->element = cell;
            variable->type->dimension_count = dimension_count;
        }

        base->variable = variable;
        base->type = variable->type;
        argument->inclusive = 1; /* marca: aloca antes de ler */

        return;
    }

    Type *type = check_array(base);

    if (type->dimension_count != dimension_count)
    {
        FAIL_AT_NODE(argument, "'%s' tem %d dimensão(ões), mas a leitura usa %d",
                     base->kind == EXPR_NAME ? base->text : "vetor", type->dimension_count,
                     dimension_count);
    }

    resolve_unknown(type->element);
}

/* ---------- comandos ---------- */

static void check_assignment(Statement *stmt)
{
    Expression *target = stmt->first;

    if (stmt->second->kind == EXPR_STRING)
    {
        FAIL_AT_NODE(stmt->second, "texto por enquanto só pode aparecer em escrever()");
    }

    Type *value_type = check_expression(stmt->second);

    if (value_type->kind == TYPE_VOID)
    {
        FAIL_AT_NODE(stmt->second, "isto não devolve valor");
    }

    if (target->kind == EXPR_TUPLE)
    {
        if (value_type->kind != TYPE_TUPLE || value_type->field_count != target->arguments.count)
        {
            FAIL_AT_NODE(stmt->second, "esperava uma tupla de %d valores", target->arguments.count);
        }

        for (int i = 0; i < target->arguments.count; i++)
        {
            Expression *item = target->arguments.items[i];
            Type *field_type = value_type->fields[i];

            if (item->kind == EXPR_NAME)
            {
                Variable *variable = lookup_variable(item->text);

                if (!variable)
                {
                    variable = declare_variable(item->text, owned_type(field_type), item->line);
                }
                else
                {
                    unify_types(variable->type, field_type, stmt->second);
                }

                item->variable = variable;
                item->type = variable->type;
                continue;
            }

            Type *type = check_expression(item);

            if (type->kind == TYPE_ARRAY)
            {
                FAIL_AT_NODE(item, "não dá para atribuir a uma linha inteira");
            }

            unify_types(type, field_type, stmt->second);
        }

        return;
    }

    if (target->kind == EXPR_NAME)
    {
        Variable *variable = lookup_variable(target->text);

        if (!variable)
        {
            variable = declare_variable(target->text, owned_type(value_type), target->line);
            target->variable = variable;
            target->type = variable->type;
            return;
        }

        target->variable = variable;
        target->type = variable->type;
        unify_types(variable->type, value_type, stmt->second);
        return;
    }

    Type *type = check_expression(target);

    if (type->kind == TYPE_ARRAY)
    {
        FAIL_AT_NODE(target,
                     "não dá para atribuir a uma linha inteira; atribua elemento por elemento");
    }

    unify_types(type, value_type, stmt->second);
}

/* declara (ou confere) a variável de um laço 'para' */
static Variable *loop_variable(const char *name, Type *type, Statement *stmt)
{
    Variable *variable = lookup_loop_variable(name);

    if (!variable)
    {
        return declare_variable(name, owned_type(type), stmt->line);
    }

    unify_types(variable->type, type, stmt->first);
    return variable;
}

/** para x em v, para (a, b) em v, para k em m, para (k, x) em m,
 * para v em g.vizinhos(u) e para (v, w) em g.vizinhos(u). */
static void check_for_in(Statement *stmt)
{
    Expression *iterable = stmt->first;
    int pattern_count = stmt->parameters.count;
    Type *types[8];
    int type_count = 0;

    if (iterable->kind == EXPR_METHOD && check_expression(iterable->first)->kind == TYPE_GRAPH &&
        method_of(TYPE_GRAPH, iterable->text, program->language) == METHOD_VIZINHOS)
    {
        Type *graph_type = iterable->first->type;

        check_argument_count(iterable, 1, 1);
        check_int(iterable->arguments.items[0]);
        iterable->type = graph_type;
        types[type_count++] = &INT_TYPE;

        if (pattern_count == 2)
        {
            graph_type->weighted = 1;
            types[type_count++] = resolve_unknown(graph_type->element);
        }
    }
    else
    {
        Type *type = check_expression(iterable);

        if (type->kind == TYPE_MAP)
        {
            types[type_count++] = resolve_unknown(type->element);

            if (pattern_count == 2)
            {
                types[type_count++] = resolve_unknown(type->value);
            }
        }
        else if (type->kind == TYPE_ARRAY)
        {
            Type *element_type = type->dimension_count == 1
                                     ? resolve_unknown(type->element)
                                     : array_type(type->element, type->dimension_count - 1);

            if (pattern_count == 0)
            {
                types[type_count++] = element_type;
            }
            else if (element_type->kind == TYPE_TUPLE &&
                     element_type->field_count == pattern_count && pattern_count <= 8)
            {
                for (int i = 0; i < pattern_count; i++)
                {
                    types[type_count++] = element_type->fields[i];
                }
            }
            else
            {
                FAIL_AT_NODE(stmt, "os elementos não são tuplas de %d valores", pattern_count);
            }
        }
        else
        {
            FAIL_AT_NODE(iterable, "não dá para percorrer %s com 'para ... em'", type_name(type));
        }
    }

    if (pattern_count == 0)
    {
        stmt->variable = loop_variable(stmt->text, types[0], stmt);
    }
    else
    {
        if (pattern_count != type_count)
        {
            FAIL_AT_NODE(stmt, "o laço dá %d valor(es), mas o padrão tem %d nomes", type_count,
                         pattern_count);
        }

        stmt->pattern_variables = allocate_zeroed(sizeof(Variable *) * (size_t)pattern_count);

        for (int i = 0; i < pattern_count; i++)
        {
            stmt->pattern_variables[i] = loop_variable(stmt->parameters.items[i], types[i], stmt);
        }
    }

    loop_depth++;
    check_statements(&stmt->body, 0);
    loop_depth--;
}

/** Checa um comando. */
static void check_statement(Statement *stmt, int top, int *seen_code)
{
    if (stmt->kind != STMT_DECL && stmt->kind != STMT_FUNCTION)
    {
        *seen_code = 1;
    }

    switch (stmt->kind)
    {
    case STMT_FUNCTION:
        if (current_instance || !top)
        {
            FAIL_AT_NODE(stmt, "funções só podem ser declaradas no topo do arquivo");
        }

        return;

    case STMT_DECL:
        if (!top || *seen_code)
        {
            FAIL_AT_NODE(stmt, "#decl só pode aparecer no começo do arquivo ou de uma função");
        }

        for (int i = 0; i < stmt->declarations.count; i++)
        {
            Declaration *declaration = &stmt->declarations.items[i];

            for (int j = 0; j < function_scope->variables.count; j++)
            {
                if (!strcmp(function_scope->variables.items[j]->name, declaration->name))
                {
                    fail_at(declaration->line, declaration->column, "'%s' já foi declarada",
                            declaration->name);
                }
            }

            if (declaration->dimensions.count > 4)
            {
                fail_at(declaration->line, declaration->column, "no máximo 4 dimensões");
            }

            Type *cell = new_type(TYPE_UNKNOWN);

            if (declaration->type_name)
            {
                Builtin builtin = builtin_of(declaration->type_name, program->language);

                if (builtin != BUILTIN_INT && builtin != BUILTIN_REAL && builtin != BUILTIN_INT32)
                {
                    fail_at(declaration->line, declaration->column,
                            "tipo desconhecido '%s' (use int, int32 ou real)",
                            declaration->type_name);
                }

                cell->kind = builtin == BUILTIN_REAL ? TYPE_REAL : TYPE_INT;
                cell->is_int32 = builtin == BUILTIN_INT32;
            }

            Variable *variable = declare_variable(
                declaration->name,
                declaration->dimensions.count ? array_type(cell, declaration->dimensions.count)
                                              : cell,
                declaration->line);

            if (declaration->dimensions.count)
            {
                variable->is_lazy = 1;
                variable->dimensions = declaration->dimensions;
                PUSH(pending_dimensions, variable);
            }
        }

        return;

    case STMT_EXPR:
    {
        Expression *expr = stmt->first;
        Builtin builtin =
            expr->kind == EXPR_CALL ? builtin_of(expr->text, program->language) : BUILTIN_NONE;

        if (builtin == BUILTIN_LER)
        {
            if (!expr->arguments.count)
            {
                FAIL_AT_NODE(expr, "ler() precisa de pelo menos uma variável");
            }

            for (int i = 0; i < expr->arguments.count; i++)
            {
                check_read_argument(expr->arguments.items[i]);
            }

            return;
        }

        if (builtin == BUILTIN_ESCREVER)
        {
            for (int i = 0; i < expr->arguments.count; i++)
            {
                Expression *argument = expr->arguments.items[i];

                if (argument->kind == EXPR_GENERATOR)
                {
                    FAIL_AT_NODE(argument, "gerador só pode ser usado em soma/min/max");
                }

                Type *type = check_expression(argument);

                if (type->kind == TYPE_VOID)
                {
                    FAIL_AT_NODE(argument, "isto não devolve valor");
                }

                if (type->kind == TYPE_HEAP || type->kind == TYPE_QUEUE || type->kind == TYPE_MAP ||
                    type->kind == TYPE_GRAPH)
                {
                    FAIL_AT_NODE(argument,
                                 "escrever() não mostra %s inteiro: percorra com 'para ... em'",
                                 type_name(type));
                }

                if (type->kind == TYPE_ARRAY)
                {
                    resolve_unknown(type->element);
                }
                else
                {
                    resolve_unknown(type);
                }
            }

            return;
        }

        if (expr->kind != EXPR_CALL && expr->kind != EXPR_METHOD)
        {
            FAIL_AT_NODE(expr, "esta expressão sozinha não faz nada");
        }

        check_expression(expr);
        return;
    }

    case STMT_ASSIGN:
        check_assignment(stmt);
        return;

    case STMT_IF:
        check_number(stmt->first);
        check_statements(&stmt->body, 0);
        check_statements(&stmt->else_body, 0);
        return;

    case STMT_WHILE:
        check_number(stmt->first);
        loop_depth++;
        check_statements(&stmt->body, 0);
        loop_depth--;
        return;

    case STMT_FOR:
    {
        check_int(stmt->first);
        check_int(stmt->second);
        Variable *variable = lookup_loop_variable(stmt->text);

        if (!variable)
        {
            variable = declare_variable(stmt->text, &INT_TYPE, stmt->line);
        }

        if (variable->type->kind == TYPE_UNKNOWN)
        {
            variable->type->kind = TYPE_INT;
        }

        if (variable->type->kind != TYPE_INT)
        {
            FAIL_AT_NODE(stmt, "a variável do para precisa ser inteira, '%s' é %s", stmt->text,
                         type_name(variable->type));
        }

        stmt->variable = variable;
        loop_depth++;
        check_statements(&stmt->body, 0);
        loop_depth--;
        return;
    }

    case STMT_FOR_IN:
        check_for_in(stmt);
        return;

    case STMT_BREAK:
    case STMT_CONTINUE:
        if (!loop_depth)
        {
            FAIL_AT_NODE(stmt, "%s fora de um laço",
                         stmt->kind == STMT_BREAK ? "pare" : "continue");
        }

        return;

    case STMT_RETURN:
    {
        if (!current_instance)
        {
            if (stmt->first)
            {
                FAIL_AT_NODE(
                    stmt, "ret com valor fora de função (no programa principal use só 'ret' para "
                          "encerrar)");
            }

            return;
        }

        Type *type = stmt->first ? check_expression(stmt->first) : &VOID_TYPE;

        if (stmt->first && (type->kind == TYPE_VOID || type->kind == TYPE_STRING))
        {
            FAIL_AT_NODE(stmt->first, "não dá para retornar %s", type_name(type));
        }

        type = stmt->first ? owned_type(type) : type;

        if (!current_instance->return_type)
        {
            current_instance->return_type = type;
        }
        else if (current_instance->return_type->kind == TYPE_VOID || type->kind == TYPE_VOID)
        {
            if (current_instance->return_type->kind != type->kind)
            {
                FAIL_AT_NODE(stmt, "'%s' retorna valor em um ponto e nada em outro",
                             current_instance->function->name);
            }
        }
        else if (current_instance->return_type->kind != type->kind &&
                 !(current_instance->return_type->kind == TYPE_REAL && type->kind == TYPE_INT))
        {
            FAIL_AT_NODE(stmt, "'%s' retorna %s em um ponto e %s em outro",
                         current_instance->function->name, type_name(current_instance->return_type),
                         type_name(type));
        }
        else if (type->kind == TYPE_ARRAY)
        {
            unify_types(current_instance->return_type, type, stmt->first);
        }

        return;
    }
    }
}

/** Checa uma lista de comandos (top: começo de arquivo ou de função, onde #decl vale). */
static void check_statements(StatementList *statements, int top)
{
    int seen_code = 0;

    for (int i = 0; i < statements->count; i++)
    {
        check_statement(statements->items[i], top, &seen_code);
    }
}

/** Checagem semântica do programa todo: escopos, tipos e versões das funções. */
void check_program(Program *parsed_program)
{
    program = parsed_program;
    current_scope = &global_scope;
    function_scope = &global_scope;
    current_instance = NULL;

    for (int i = 0; i < parsed_program->statements.count; i++)
    {
        Statement *stmt = parsed_program->statements.items[i];

        if (stmt->kind != STMT_FUNCTION)
        {
            continue;
        }

        if (find_function(stmt->text))
        {
            FAIL_AT_NODE(stmt, "a função '%s' foi declarada duas vezes", stmt->text);
        }

        if (builtin_of(stmt->text, parsed_program->language))
        {
            FAIL_AT_NODE(stmt, "'%s' é um nome nativo da linguagem", stmt->text);
        }

        Function *function = allocate_zeroed(sizeof *function);
        function->name = stmt->text;
        function->definition = stmt;
        PUSH(functions, function);
    }

    check_statements(&parsed_program->statements, 1);
    check_pending_dimensions(0);
}
