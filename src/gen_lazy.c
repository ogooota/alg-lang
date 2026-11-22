#include "gen_internal.h"

/** Diz se a variável está no conjunto. */
int set_contains(VariableSet *set, Variable *variable)
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
int set_add(VariableSet *set, Variable *variable)
{
    if (set_contains(set, variable))
    {
        return 0;
    }

    PUSH(*set, variable);
    return 1;
}

/** Escreve a criação de um vetor do #decl com os tamanhos dele. */
void emit_lazy_allocation(TextBuffer *out, Variable *variable, int line)
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
void emit_lazy_ensure(TextBuffer *out, Variable *variable, int line)
{
    text_append(out, "(");
    emit_identifier(out, "v_", variable->name);
    text_append(out, ".data || (");
    emit_identifier(out, "v_", variable->name);
    text_append(out, " = ");
    emit_lazy_allocation(out, variable, line);
    text_append(out, ", 1))");
}

/* lazy: vetores do #decl usados; writes: variáveis alteradas (inclusive por funções chamadas) */
void walk_expression(Expression *expr, VariableSet *lazy_arrays, VariableSet *writes)
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
void walk_statement(Statement *stmt, VariableSet *lazy_arrays, VariableSet *writes)
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
void walk_statements(StatementList *statements, VariableSet *lazy_arrays, VariableSet *writes)
{
    for (int i = 0; i < statements->count; i++)
    {
        walk_statement(statements->items[i], lazy_arrays, writes);
    }
}

/* o que cada função altera, repetindo até estabilizar (recursão mútua) */
void compute_writes(Program *parsed_program)
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
int dimensions_change_in(Variable *variable, VariableSet *writes)
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

/* variável do programa principal citada em alguma função precisa ser global no C */
void mark_expression(Expression *expr)
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
void mark_statements(StatementList *statements)
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
