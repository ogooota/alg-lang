#include "gen_internal.h"

/* ---------- comandos ---------- */

void gen_block(TextBuffer *out, StatementList *statements, int depth)
{
    indent(out, depth);
    text_append(out, "{\n");
    gen_statements(out, statements, depth + 1);
    indent(out, depth);
    text_append(out, "}\n");
}

/** Escreve um laço com a checagem do #decl feita uma vez antes dele, quando é seguro. */
void gen_loop_hoisted(TextBuffer *out, Statement *stmt, int depth)
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
void gen_assign_line(TextBuffer *out, Variable *variable, int line, int depth,
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
void gen_for_in(TextBuffer *out, Statement *stmt, int depth)
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
        text_appendf(out, "%s_Entries _a%d = %s_entries(_m%d);\n", c_name, temp_id, c_name,
                     temp_id);
        indent(out, depth + 1);
        text_appendf(out, "for (int64_t _k%d = 0; _k%d < _a%d.length; _k%d++)\n", temp_id, temp_id,
                     temp_id, temp_id);
        indent(out, depth + 1);
        text_append(out, "{\n");

        const char *loader = cell_element_kind(iterable_type->element)->loader;
        const char *value_loader =
            cell_root(iterable_type->value)->kind == TYPE_STRING ? "str_unpack" : "";

        if (stmt->pattern_variables)
        {
            /* o valor sai da foto enquanto o corpo não escreve no mapa; depois, busca de novo */
            gen_assign_line(out, stmt->pattern_variables[0], stmt->line, body_indent,
                            "%s(_a%d.data[_k%d].key)", loader, temp_id, temp_id);
            gen_assign_line(out, stmt->pattern_variables[1], stmt->line, body_indent,
                            "%s(_a%d.writes == _m%d->writes ? _a%d.data[_k%d].value : "
                            "%s_get(_m%d, %s(_a%d.data[_k%d].key)))",
                            value_loader, temp_id, temp_id, temp_id, temp_id, c_name, temp_id, loader,
                            temp_id, temp_id);
        }
        else
        {
            gen_assign_line(out, stmt->variable, stmt->line, body_indent,
                            "%s(_a%d.data[_k%d].key)", loader, temp_id, temp_id);
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
void gen_tuple_assignment(TextBuffer *out, Statement *stmt, int depth)
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
                         generated_program->uses_inf ? "encode_int32_inf" : "fit_int32", temp_id, i,
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
void gen_loop(TextBuffer *out, Statement *stmt, int depth)
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
void gen_statement(TextBuffer *out, Statement *stmt, int depth)
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

        if (builtin == BUILTIN_LER_LINHA)
        {
            indent(out, depth);
            gen_variable(out, expr->arguments.items[0]->variable, expr->line);
            text_appendf(out, " = str_read_line(%d);\n", expr->line);
            return;
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
        else if (stmt->first->kind == EXPR_INDEX && stmt->first->first->type->kind == TYPE_MAP &&
                 stmt->second->kind == EXPR_BINARY && stmt->second->first == stmt->first)
        {
            /* m[k] += x: uma busca só (o ponteiro do valor serve para ler e escrever) */
            int temp_id = temp_counter++;
            Type *map_type = stmt->first->first->type;

            text_appendf(out, "{ %s *_p%d = &", structure_template(map_type)->value->c_type, temp_id);
            gen_lvalue(out, stmt->first);
            text_appendf(out, "; ");
            cached_map_index = stmt->first;
            cached_map_pointer = temp_id;
            text_appendf(out, "*_p%d = ", temp_id);
            gen_stored_value(out, stmt->first->type, stmt->second);
            cached_map_index = NULL;
            text_append(out, "; }\n");
            return;
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
void gen_statements(TextBuffer *out, StatementList *statements, int depth)
{
    for (int i = 0; i < statements->count; i++)
    {
        gen_statement(out, statements->items[i], depth);
    }
}

/* ---------- programa ---------- */

void gen_prototype(TextBuffer *out, FunctionInstance *instance)
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
void gen_variable_declaration(TextBuffer *out, Variable *variable, int depth, const char *prefix)
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
