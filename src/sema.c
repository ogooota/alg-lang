#include "sema_internal.h"

Program *checked_program;
FunctionList functions;
int loop_depth;
/* escopos: global, o da função atual e o atual (geradores abrem um escopo filho) */
Scope global_scope;
Scope *function_scope;
Scope *current_scope;
/* instância de função sendo checada (NULL = programa principal) */
FunctionInstance *current_instance;
/* #decl v[n]: as dimensões são checadas no fim do escopo, quando n já existe */
VariableList pending_dimensions;
Type INT_TYPE = {.kind = TYPE_INT};
Type REAL_TYPE = {.kind = TYPE_REAL};
Type STRING_TYPE = {.kind = TYPE_STRING};
Type VOID_TYPE = {.kind = TYPE_VOID};
Type CHAR_TYPE = {.kind = TYPE_CHAR};

/* ---------- ler / escrever ---------- */

void check_read_argument(Expression *argument)
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
            variable->is_declared_string = 0;
            return;
        }

        if (variable->type->kind == TYPE_STRING)
        {
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

        if (!is_number(type) && !(is_string(type) && holds_strings(argument)))
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

    if (variable && variable->is_declared_string)
    {
        Type *cell = new_type(TYPE_STRING);

        variable->is_declared_string = 0;
        variable->type = array_type(cell, dimension_count);
        base->variable = variable;
        base->type = variable->type;
        argument->inclusive = 1;
        return;
    }

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

/** O alvo é uma posição de vetor de strings ou um valor de mapa de strings. */
int holds_strings(Expression *target)
{
    if (target->kind != EXPR_INDEX)
    {
        return 0;
    }

    Type *container = target->first->type;
    Type *cell = container->kind == TYPE_MAP     ? container->value
                 : container->kind == TYPE_ARRAY ? container->element
                                                 : NULL;
    return cell && cell_root(cell)->kind == TYPE_STRING;
}

void check_assignment(Statement *stmt)
{
    Expression *target = stmt->first;

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
    Type *container = target->kind == EXPR_INDEX ? target->first->type : NULL;

    /* string numa posição de estrutura: a célula vira TYPE_STRING (guardada por cópia) */
    if (container && is_string(resolve_unknown(value_type)) &&
        (container->kind == TYPE_MAP ||
         (container->kind == TYPE_ARRAY && container->dimension_count == 1)))
    {
        Type *cell = container->kind == TYPE_MAP ? container->value : container->element;

        store_in_cell(cell, value_type, stmt->second);
        target->type = value_of_cell(cell);
        return;
    }

    if (type->kind == TYPE_ARRAY && !holds_strings(target))
    {
        FAIL_AT_NODE(target,
                     "não dá para atribuir a uma linha inteira; atribua elemento por elemento");
    }

    unify_types(type, value_type, stmt->second);
}

/* declara (ou confere) a variável de um laço 'para' */
Variable *loop_variable(const char *name, Type *type, Statement *stmt)
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
void check_for_in(Statement *stmt)
{
    Expression *iterable = stmt->first;
    int pattern_count = stmt->parameters.count;
    Type *types[8];
    int type_count = 0;

    if (iterable->kind == EXPR_METHOD && check_expression(iterable->first)->kind == TYPE_GRAPH &&
        method_of(TYPE_GRAPH, iterable->text, checked_program->language) == METHOD_VIZINHOS)
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
            types[type_count++] = value_of_cell(resolve_unknown(type->element));

            if (pattern_count == 2)
            {
                types[type_count++] = value_of_cell(resolve_unknown(type->value));
            }
        }
        else if (type->kind == TYPE_ARRAY)
        {
            Type *element_type = type->dimension_count == 1
                                     ? value_of_cell(resolve_unknown(type->element))
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
void check_statement(Statement *stmt, int top, int *seen_code)
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

            if (scope_find(function_scope, declaration->name))
            {
                fail_at(declaration->line, declaration->column, "'%s' já foi declarada",
                        declaration->name);
            }

            if (declaration->dimensions.count > 4)
            {
                fail_at(declaration->line, declaration->column, "no máximo 4 dimensões");
            }

            Type *cell = new_type(TYPE_UNKNOWN);

            if (declaration->type_name)
            {
                Builtin builtin = builtin_of(declaration->type_name, checked_program->language);

                if (builtin != BUILTIN_INT && builtin != BUILTIN_REAL && builtin != BUILTIN_INT32 &&
                    builtin != BUILTIN_TEXTO)
                {
                    fail_at(declaration->line, declaration->column,
                            "tipo desconhecido '%s' (use int, int32, real ou str)",
                            declaration->type_name);
                }

                cell->kind = builtin == BUILTIN_REAL    ? TYPE_REAL
                             : builtin == BUILTIN_TEXTO ? TYPE_STRING
                                                        : TYPE_INT;
                cell->is_int32 = builtin == BUILTIN_INT32;
            }

            /* #decl s: str é uma string (ler(s) lê palavra); com [n], vetor de strings */
            int is_text = cell->kind == TYPE_STRING && !declaration->dimensions.count;
            Variable *variable = declare_variable(
                declaration->name,
                declaration->dimensions.count ? array_type(cell, declaration->dimensions.count)
                : is_text                     ? new_string_type()
                                              : cell,
                declaration->line);

            variable->is_declared_string = is_text;

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
        Builtin builtin = expr->kind == EXPR_CALL
                              ? builtin_of(expr->text, checked_program->language)
                              : BUILTIN_NONE;

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

        if (builtin == BUILTIN_LER_LINHA)
        {
            check_argument_count(expr, 1, 1);
            Expression *target = expr->arguments.items[0];

            if (target->kind != EXPR_NAME)
            {
                FAIL_AT_NODE(target, "ler_linha() lê numa variável de texto");
            }

            Variable *variable = lookup_variable(target->text);

            if (!variable)
            {
                variable = declare_variable(target->text, new_string_type(), target->line);
            }

            if (!is_string(variable->type))
            {
                FAIL_AT_NODE(target, "'%s' é %s, e ler_linha() lê texto", target->text,
                             type_name(variable->type));
            }

            variable->is_declared_string = 0;
            target->variable = variable;
            target->type = variable->type;
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
        check_statements(&stmt->else_body, 0);
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
void check_statements(StatementList *statements, int top)
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
    checked_program = parsed_program;
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
