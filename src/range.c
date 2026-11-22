#include "alg.h"

/* Análise de faixa: o menor e o maior valor de cada vetor, para escolher 1, 2, 4 ou 8 bytes.
 * Só estreita o que está provado; invariantes em docs/compilador.md. */

typedef struct
{
    int64_t low;
    int64_t high;
} Range;

#define MAX_GROWTHS 4

static const Range FULL_RANGE = {INT64_MIN, INT64_MAX};
static const Range BOOL_RANGE = {0, 1};
static int changed;
int narrowing_ok = 1;

/** Faixa [low, high]. */
static Range make_range(int64_t lo, int64_t hi)
{
    Range range = {lo, hi};
    return range;
}

/** Menor faixa que contém as duas. */
static Range join_ranges(Range left, Range right)
{
    return make_range(left.low < right.low ? left.low : right.low,
                      left.high > right.high ? left.high : right.high);
}

/* ---------- onde as faixas ficam guardadas ---------- */

/* alarga a faixa (lo, hi) com r; devolve a faixa nova */
static void widen_range(int64_t *lo, int64_t *hi, int *growths, Range range)
{
    if (range.low >= *lo && range.high <= *hi)
    {
        return;
    }

    (*growths)++;
    changed = 1;

    if (*growths > MAX_GROWTHS)
    {
        range = FULL_RANGE;
    }

    Range joined = join_ranges(make_range(*lo, *hi), range);
    *lo = joined.low;
    *hi = joined.high;
}

/** Faixa dos inteiros de 32 bits. */
static Range int32_range(void)
{
    return make_range(INT32_MIN, INT32_MAX);
}

/** Uma escrita na célula de vetor: alarga a faixa dela. */
static void store_cell_range(Type *cell, Range range)
{
    Type *root = cell_root(cell);

    if (root->kind != TYPE_INT)
    {
        return;
    }

    /* int32 anotado: o que não cabe vira erro (ou é cortado com -O) */
    if (root->is_int32)
    {
        Range limit = int32_range();
        range.low = range.low < limit.low ? limit.low : range.low;
        range.high = range.high > limit.high ? limit.high : range.high;
    }

    widen_range(&root->low, &root->high, &root->growths, range);
}

/** Faixa atual de uma célula de vetor (tudo, se não for inteira). */
static Range cell_range(Type *cell)
{
    Type *root = cell_root(cell);

    if (root->kind != TYPE_INT)
    {
        return FULL_RANGE;
    }

    return make_range(root->low, root->high);
}

/** Uma escrita na variável: alarga a faixa dela. */
static void store_variable_range(Variable *variable, Range range)
{
    if (variable && variable->type->kind == TYPE_INT)
    {
        widen_range(&variable->low, &variable->high, &variable->growths, range);
    }
}

/** Faixa atual de uma variável (tudo para parâmetros). */
static Range variable_range(Variable *variable)
{
    if (!variable || variable->type->kind != TYPE_INT || variable->is_parameter)
    {
        return FULL_RANGE;
    }

    return make_range(variable->low, variable->high);
}

/* ---------- aritmética de intervalos ---------- */

static Range range_add(Range left, Range right)
{
    Range range;

    if (__builtin_add_overflow(left.low, right.low, &range.low) ||
        __builtin_add_overflow(left.high, right.high, &range.high))
    {
        return FULL_RANGE;
    }

    return range;
}

/** Faixa de a - b (tudo se pode estourar). */
static Range range_subtract(Range left, Range right)
{
    Range range;

    if (__builtin_sub_overflow(left.low, right.high, &range.low) ||
        __builtin_sub_overflow(left.high, right.low, &range.high))
    {
        return FULL_RANGE;
    }

    return range;
}

/** Faixa de a * b (tudo se pode estourar). */
static Range range_multiply(Range left, Range right)
{
    int64_t candidates[4];

    if (__builtin_mul_overflow(left.low, right.low, &candidates[0]) ||
        __builtin_mul_overflow(left.low, right.high, &candidates[1]) ||
        __builtin_mul_overflow(left.high, right.low, &candidates[2]) ||
        __builtin_mul_overflow(left.high, right.high, &candidates[3]))
    {
        return FULL_RANGE;
    }

    Range range = make_range(candidates[0], candidates[0]);

    for (int i = 1; i < 4; i++)
    {
        range = join_ranges(range, make_range(candidates[i], candidates[i]));
    }

    return range;
}

/** Divisão que arredonda para baixo. */
static int64_t floor_divide(int64_t dividend, int64_t divisor)
{
    int64_t quotient = dividend / divisor;

    if ((dividend % divisor != 0) && ((dividend < 0) != (divisor < 0)))
    {
        quotient--;
    }

    return quotient;
}

/* divisão que arredonda para baixo; só calculamos quando o divisor não cruza o zero */
static Range range_divide(Range left, Range right)
{
    if (right.low <= 0 && right.high >= 0)
    {
        return FULL_RANGE;
    }

    if (left.low == INT64_MIN || right.low == INT64_MIN)
    {
        return FULL_RANGE;
    }

    int64_t candidates[4] = {floor_divide(left.low, right.low), floor_divide(left.low, right.high),
                             floor_divide(left.high, right.low),
                             floor_divide(left.high, right.high)};
    Range range = make_range(candidates[0], candidates[0]);

    for (int i = 1; i < 4; i++)
    {
        range = join_ranges(range, make_range(candidates[i], candidates[i]));
    }

    return range;
}

/* resto euclidiano: sempre em [0, |b| - 1] */
static Range range_modulo(Range left, Range right)
{
    if (right.low == INT64_MIN)
    {
        return make_range(0, INT64_MAX);
    }

    int64_t modulus = right.high > -right.low ? right.high : -right.low;

    if (modulus <= 0)
    {
        return make_range(0, 0);
    }

    Range range = make_range(0, modulus - 1);

    if (left.low >= 0 && left.high < range.high)
    {
        range.high = left.high;
    }

    return range;
}

/* menor 2^k - 1 que cobre x (para | e ^) */
static int64_t fill_bits(int64_t number)
{
    uint64_t value = (uint64_t)number;

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return (int64_t)value;
}

/** Faixa de &, |, ^, << e >>. */
static Range range_bitwise(TokenKind operation, Range left, Range right)
{
    if (operation == PUNCT_AMPERSAND)
    {
        if (left.low >= 0 && right.low >= 0)
        {
            return make_range(0, left.high < right.high ? left.high : right.high);
        }

        if (left.low >= 0)
        {
            return make_range(0, left.high);
        }

        if (right.low >= 0)
        {
            return make_range(0, right.high);
        }

        return FULL_RANGE;
    }

    if (operation == PUNCT_BAR || operation == PUNCT_CARET)
    {
        if (left.low >= 0 && right.low >= 0)
        {
            return make_range(0, fill_bits(left.high > right.high ? left.high : right.high));
        }

        return FULL_RANGE;
    }

    if (operation == PUNCT_SHIFT_RIGHT)
    {
        if (left.low >= 0 && right.low >= 0)
        {
            return make_range(0, left.high);
        }

        return FULL_RANGE;
    }

    /* << */
    if (left.low >= 0 && right.low >= 0 && right.high < 63 &&
        left.high <= (INT64_MAX >> right.high))
    {
        return make_range(left.low << right.low, left.high << right.high);
    }

    return FULL_RANGE;
}

/* ---------- faixa de uma expressão ---------- */

static int is_int_expression(Expression *expr)
{
    return expr->type && expr->type->kind == TYPE_INT;
}

/** Nome nativo em qualquer idioma (o range.c não sabe o idioma do arquivo). */
static Builtin builtin_any(const char *name)
{
    Builtin builtin = builtin_of(name, LANG_PT);
    return builtin ? builtin : builtin_of(name, LANG_EN);
}

static Range expression_range(Expression *expr);

/** Faixa de min/max/soma. */
static Range reduce_range(Expression *expr, Builtin builtin)
{
    if (builtin == BUILTIN_SOMA)
    {
        return FULL_RANGE;
    }

    if (expr->arguments.count == 1)
    {
        Expression *argument = expr->arguments.items[0];

        if (argument->kind == EXPR_GENERATOR)
        {
            return expression_range(argument->first);
        }

        return cell_range(argument->type->element);
    }

    Range range = expression_range(expr->arguments.items[0]);

    for (int i = 1; i < expr->arguments.count; i++)
    {
        Range left = expression_range(expr->arguments.items[i]);

        if (builtin == BUILTIN_MIN)
        {
            range = make_range(range.low < left.low ? range.low : left.low,
                               range.high < left.high ? range.high : left.high);
        }
        else
        {
            range = make_range(range.low > left.low ? range.low : left.low,
                               range.high > left.high ? range.high : left.high);
        }
    }

    return range;
}

/** Faixa de valores que a expressão pode ter. */
static Range expression_range(Expression *expr)
{
    if (!is_int_expression(expr))
    {
        return FULL_RANGE;
    }

    switch (expr->kind)
    {
    case EXPR_INT:
        return make_range(expr->int_value, expr->int_value);

    case EXPR_NAME:
        return variable_range(expr->variable);

    case EXPR_INDEX:

        /* só vetor tem faixa conhecida; mapa devolve qualquer coisa */
        return expr->first->type->kind == TYPE_ARRAY ? cell_range(expr->type) : FULL_RANGE;

    case EXPR_UNARY:
        if (expr->operation == KEYWORD_NAO)
        {
            return BOOL_RANGE;
        }

        Range left = expression_range(expr->first);

        if (expr->operation == PUNCT_TILDE)
        {
            return make_range(~left.high, ~left.low);
        }

        return left.low == INT64_MIN ? FULL_RANGE : make_range(-left.high, -left.low);

    case EXPR_BINARY:
    {
        if (expr->operation == KEYWORD_E || expr->operation == KEYWORD_OU ||
            (expr->operation >= PUNCT_EQUAL && expr->operation <= PUNCT_GREATER_EQUAL))
        {
            return BOOL_RANGE;
        }

        Range left = expression_range(expr->first);
        Range right = expression_range(expr->second);

        switch (expr->operation)
        {
        case PUNCT_PLUS:
            return range_add(left, right);

        case PUNCT_MINUS:
            return range_subtract(left, right);

        case PUNCT_STAR:
            return range_multiply(left, right);

        case PUNCT_SLASH:
            return range_divide(left, right);

        case PUNCT_PERCENT:
            return range_modulo(left, right);

        default:
            return range_bitwise(expr->operation, left, right);
        }
    }

    case EXPR_CALL:
    {
        if (expr->instance)
        {
            return FULL_RANGE;
        }

        Builtin builtin = builtin_any(expr->text);

        if (builtin == BUILTIN_MIN || builtin == BUILTIN_MAX || builtin == BUILTIN_SOMA)
        {
            return reduce_range(expr, builtin);
        }

        if (builtin == BUILTIN_TAMANHO)
        {
            return make_range(0, INT64_MAX);
        }

        if ((builtin == BUILTIN_INT || builtin == BUILTIN_INT32) &&
            is_int_expression(expr->arguments.items[0]))
        {
            return expression_range(expr->arguments.items[0]);
        }

        return FULL_RANGE;
    }

    case EXPR_METHOD:

        /* remover_ultimo devolve um elemento do vetor; heap/fila: valor de fora */
        if (expr->first->type->kind == TYPE_ARRAY)
        {
            return cell_range(expr->first->type->element);
        }

        return FULL_RANGE;

    default:
        return FULL_RANGE;
    }
}

/* ---------- escritas ---------- */

static void walk_statements(StatementList *statements);

/* escritas que acontecem dentro de expressões: vetor literal, adicionar, gerador */
static void walk_expression(Expression *expr)
{
    if (!expr)
    {
        return;
    }

    if (expr->kind == EXPR_VECTOR && expr->type->element->kind == TYPE_INT)
    {
        for (int i = 0; i < expr->arguments.count; i++)
        {
            store_cell_range(expr->type->element, expression_range(expr->arguments.items[i]));
        }
    }

    if (expr->kind == EXPR_METHOD && builtin_any(expr->text) == BUILTIN_ADICIONAR)
    {
        store_cell_range(expr->first->type->element, expression_range(expr->arguments.items[0]));
    }

    if (expr->kind == EXPR_GENERATOR && expr->variable->type->kind == TYPE_INT)
    {
        if (expr->second)
        {
            store_variable_range(expr->variable, join_ranges(expression_range(expr->second),
                                                             expression_range(expr->third)));
        }
        else
        {
            store_variable_range(expr->variable, expr->third->type->dimension_count == 1
                                                     ? cell_range(expr->third->type->element)
                                                     : FULL_RANGE);
        }
    }

    walk_expression(expr->first);
    walk_expression(expr->second);
    walk_expression(expr->third);

    for (int i = 0; i < expr->arguments.count; i++)
    {
        walk_expression(expr->arguments.items[i]);
    }
}

/* ler(...): o valor vem de fora, então a faixa é "tudo" */
static void walk_read(Expression *call_expr)
{
    for (int i = 0; i < call_expr->arguments.count; i++)
    {
        Expression *argument = call_expr->arguments.items[i];

        walk_expression(argument);

        if (argument->kind == EXPR_NAME && argument->variable->type->kind == TYPE_ARRAY)
        {
            store_cell_range(argument->variable->type->element, FULL_RANGE);
        }
        else if (argument->kind == EXPR_NAME)
        {
            store_variable_range(argument->variable, FULL_RANGE);
        }
        else if (argument->kind == EXPR_INDEX)
        {
            store_cell_range(argument->type, FULL_RANGE);
        }
        else
        {
            Expression *base = argument;

            while (base->kind == EXPR_SLICE)
            {
                base = base->first;
            }

            store_cell_range(base->type->element, FULL_RANGE);
        }
    }
}

/** Percorre um comando registrando as escritas. */
static void walk_statement(Statement *stmt)
{
    switch (stmt->kind)
    {
    case STMT_FUNCTION:
    case STMT_DECL:
    case STMT_BREAK:
    case STMT_CONTINUE:
        return;

    case STMT_EXPR:
    {
        Builtin builtin = stmt->first->kind == EXPR_CALL && !stmt->first->instance
                              ? builtin_any(stmt->first->text)
                              : BUILTIN_NONE;

        if (builtin == BUILTIN_LER)
        {
            walk_read(stmt->first);
        }
        else
        {
            walk_expression(stmt->first);
        }

        return;
    }

    case STMT_ASSIGN:
        walk_expression(stmt->first);
        walk_expression(stmt->second);

        if (stmt->first->kind == EXPR_TUPLE)
        {
            /* (a, b) = (x, y) usa a faixa de cada parte; senão, qualquer valor */
            for (int i = 0; i < stmt->first->arguments.count; i++)
            {
                Expression *part = stmt->first->arguments.items[i];
                Range range = stmt->second->kind == EXPR_TUPLE
                                  ? expression_range(stmt->second->arguments.items[i])
                                  : FULL_RANGE;

                if (part->kind == EXPR_NAME)
                {
                    store_variable_range(part->variable, range);
                }
                else
                {
                    store_cell_range(part->type, range);
                }
            }
        }
        else if (stmt->first->kind == EXPR_NAME)
        {
            store_variable_range(stmt->first->variable, expression_range(stmt->second));
        }
        else
        {
            store_cell_range(stmt->first->type, expression_range(stmt->second));
        }

        return;

    case STMT_FOR:
    {
        walk_expression(stmt->first);
        walk_expression(stmt->second);

        /* depois do laço a variável vale o limite (ou limite + 1 no "ate em") */
        Range upper = expression_range(stmt->second);

        if (stmt->inclusive)
        {
            upper = range_add(upper, make_range(1, 1));
        }

        store_variable_range(stmt->variable, join_ranges(expression_range(stmt->first), upper));
        walk_statements(&stmt->body);
        return;
    }

    case STMT_FOR_IN:
        walk_expression(stmt->first);

        if (stmt->first->type->kind == TYPE_ARRAY && stmt->first->type->dimension_count == 1)
        {
            store_variable_range(stmt->variable, cell_range(stmt->first->type->element));
        }
        else
        {
            store_variable_range(stmt->variable, FULL_RANGE);
        }

        for (int i = 0; stmt->pattern_variables && i < stmt->parameters.count; i++)
        {
            store_variable_range(stmt->pattern_variables[i], FULL_RANGE);
        }

        walk_statements(&stmt->body);
        return;

    case STMT_IF:
    case STMT_WHILE:
        walk_expression(stmt->first);
        walk_statements(&stmt->body);
        walk_statements(&stmt->else_body);
        return;

    case STMT_RETURN:
        walk_expression(stmt->first);
        return;
    }
}

/** Percorre uma lista de comandos. */
static void walk_statements(StatementList *statements)
{
    for (int i = 0; i < statements->count; i++)
    {
        walk_statement(statements->items[i]);
    }
}

/** Calcula as faixas até estabilizar (ponto fixo); sem estabilizar, desliga o estreitamento. */
void analyze_ranges(Program *parsed_program)
{
    /* cada volta só alarga faixas; o alargamento garante o fim */
    for (int round = 0; round < 64; round++)
    {
        changed = 0;
        walk_statements(&parsed_program->statements);

        for (int i = 0; i < parsed_program->instances.count; i++)
        {
            walk_statements(&parsed_program->instances.items[i]->body);
        }

        if (!changed)
        {
            return;
        }
    }

    /* não estabilizou (não deveria acontecer): sem prova, todo vetor fica com 8 bytes */
    narrowing_ok = 0;
}
