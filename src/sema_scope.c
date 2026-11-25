#include "sema_internal.h"

/** Hash FNV-1a do nome. */
uint64_t name_hash(const char *name)
{
    uint64_t hash = 1469598103934665603ULL;

    for (const unsigned char *cursor = (const unsigned char *)name; *cursor; cursor++)
    {
        hash = (hash ^ *cursor) * 1099511628211ULL;
    }

    return hash;
}

/** Variável com esse nome declarada neste escopo (sem olhar os de fora); NULL se não há. */
Variable *scope_find(Scope *scope, const char *name)
{
    if (!scope->capacity)
    {
        return NULL;
    }

    uint64_t mask = (uint64_t)scope->capacity - 1;

    for (uint64_t slot = name_hash(name) & mask; scope->slots[slot]; slot = (slot + 1) & mask)
    {
        if (!strcmp(scope->slots[slot]->name, name))
        {
            return scope->slots[slot];
        }
    }

    return NULL;
}

/** Põe a variável na tabela do escopo, dobrando a tabela quando passa da metade. */
void scope_insert(Scope *scope, Variable *variable)
{
    if ((scope->variables.count + 1) * 2 > scope->capacity)
    {
        int capacity = scope->capacity ? scope->capacity * 2 : 16;
        Variable **slots = allocate_zeroed(sizeof(Variable *) * (size_t)capacity);

        for (int i = 0; i < scope->capacity; i++)
        {
            Variable *old = scope->slots[i];

            if (old)
            {
                uint64_t slot = name_hash(old->name) & (uint64_t)(capacity - 1);

                while (slots[slot])
                {
                    slot = (slot + 1) & (uint64_t)(capacity - 1);
                }

                slots[slot] = old;
            }
        }

        free(scope->slots);
        scope->slots = slots;
        scope->capacity = capacity;
    }

    uint64_t mask = (uint64_t)scope->capacity - 1;
    uint64_t slot = name_hash(variable->name) & mask;

    while (scope->slots[slot] && strcmp(scope->slots[slot]->name, variable->name))
    {
        slot = (slot + 1) & mask;
    }

    scope->slots[slot] = variable;
}

/** Procura um nome do escopo atual para fora; NULL se não existe. */
Variable *lookup_variable(const char *name)
{
    for (Scope *scope = current_scope; scope; scope = scope->parent)
    {
        Variable *found = scope_find(scope, name);

        if (found)
        {
            return found;
        }
    }

    return NULL;
}

/*
 * Variável do 'para': dentro de função ela é sempre local. Assim um laço
 * dentro de dfs() nunca sobrescreve o 'i' do laço do programa principal.
 */
Variable *lookup_loop_variable(const char *name)
{
    if (!current_instance)
    {
        return lookup_variable(name);
    }

    for (Scope *scope = current_scope; scope; scope = scope->parent)
    {
        Variable *found = scope_find(scope, name);

        if (found)
        {
            return found;
        }

        if (scope == function_scope)
        {
            break;
        }
    }

    return NULL;
}

/** Diz se a expressão lê o nome (x = x + 1, x++, [a, b] = (b, a)). */
static int expression_mentions(Expression *expr, const char *name)
{
    if (!expr)
    {
        return 0;
    }

    if (expr->kind == EXPR_NAME && !strcmp(expr->text, name))
    {
        return 1;
    }

    for (int i = 0; i < expr->arguments.count; i++)
    {
        if (expression_mentions(expr->arguments.items[i], name))
        {
            return 1;
        }
    }

    return expression_mentions(expr->first, name) || expression_mentions(expr->second, name) ||
           expression_mentions(expr->third, name);
}

/**
 * Alvo de atribuição: dentro de função vale a local ou a global do #decl do topo; outra
 * global com o mesmo nome não é tocada (NULL: quem chama cria a local).
 */
Variable *lookup_assigned_variable(const char *name, Expression *value)
{
    if (!current_instance)
    {
        return lookup_variable(name);
    }

    Variable *local = lookup_loop_variable(name);

    if (local)
    {
        return local;
    }

    Variable *global = scope_find(&global_scope, name);

    if (global && global->is_declared)
    {
        return global;
    }

    /* x++ numa global sem #decl mudaria só uma cópia local: melhor avisar */
    if (global && expression_mentions(value, name))
    {
        FAIL_AT_NODE(value, "'%s' é global: para mudar dentro da função, declare no #decl do topo",
                     name);
    }

    return NULL;
}

/** Cria uma variável no escopo dado. */
Variable *declare_in_scope(Scope *scope, const char *name, Type *ty, int line)
{
    Variable *variable = allocate_zeroed(sizeof *variable);
    variable->name = duplicate_string(name);
    variable->type = ty;
    variable->line = line;
    scope_insert(scope, variable);
    PUSH(scope->variables, variable);
    return variable;
}

/** Cria uma variável no escopo da função atual (ou global no programa principal). */
Variable *declare_variable(const char *name, Type *ty, int line)
{
    Variable *variable = declare_in_scope(function_scope, name, ty, line);

    if (function_scope == &global_scope)
    {
        variable->is_global = 1;
        PUSH(checked_program->globals, variable);
    }
    else
    {
        PUSH(current_instance->locals, variable);
    }

    return variable;
}

/** Procura uma função do usuário pelo nome; NULL se não existe. */
Function *find_function(const char *name)
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

/** Cópia profunda de uma lista de expressões. */
void clone_expression_list(ExpressionList *destination, ExpressionList *source)
{
    *destination = (ExpressionList){0};

    for (int i = 0; i < source->count; i++)
    {
        PUSH(*destination, clone_expression(source->items[i]));
    }
}

/** Cópia profunda de uma expressão (cada versão de função anota a sua). */
Expression *clone_expression(Expression *expr)
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

/** Cópia profunda de uma lista de comandos. */
void clone_statement_list(StatementList *destination, StatementList *source)
{
    *destination = (StatementList){0};

    for (int i = 0; i < source->count; i++)
    {
        PUSH(*destination, clone_statement(source->items[i]));
    }
}

/** Cópia profunda de um comando. */
Statement *clone_statement(Statement *stmt)
{
    Statement *copy = allocate_zeroed(sizeof *copy);

    *copy = *stmt;
    copy->first = clone_expression(stmt->first);
    copy->second = clone_expression(stmt->second);
    copy->third = clone_expression(stmt->third);
    clone_statement_list(&copy->body, &stmt->body);
    clone_statement_list(&copy->else_body, &stmt->else_body);
    copy->fallback = stmt->fallback ? clone_statement(stmt->fallback) : NULL;
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

/** Checa os tamanhos dos #decl v[n] do escopo que está terminando. */
void check_pending_dimensions(int from)
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
FunctionInstance *find_instance(Function *function, Type **argument_types, int count)
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
FunctionInstance *new_instance(Function *function, Type **argument_types)
{
    Statement *definition = function->definition;
    FunctionInstance *instance = allocate_zeroed(sizeof *instance);

    instance->function = function;
    instance->parameter_types = argument_types;
    instance->is_being_checked = 1;
    instance->next = function->instances;
    function->instances = instance;

    /* fn f(...) -> tipo: os ret conferem com ele, e a recursão já sabe o retorno */
    if (definition->return_type)
    {
        instance->return_type = type_from_syntax(definition->return_type, 0);
    }

    instance->c_name = allocate_zeroed(strlen(function->name) + 24);
    sprintf(instance->c_name, "%s_%d", function->name, function->instance_count);
    function->instance_count++;

    clone_statement_list(&instance->body, &definition->body);
    PUSH(checked_program->instances, instance);

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
