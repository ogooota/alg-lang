#include "alg.h"

static TokenList *tokens;
static int position;
static Language lang;

/* > 0 quando está dentro de uma função (funções só podem ser declaradas no topo) */
static int function_depth;

/* ---------- navegação nos tokens ---------- */

static Token *current_token(void)
{
    return &tokens->items[position];
}

/** Diz se o token atual é do tipo pedido. */
static int at_token(TokenKind kind)
{
    return current_token()->kind == kind;
}

/** Consome o token atual se for do tipo pedido; devolve se consumiu. */
static int accept_token(TokenKind kind)
{
    if (at_token(kind))
    {
        position++;
        return 1;
    }

    return 0;
}

/** Consome o token do tipo pedido ou encerra com erro. */
static Token *expect_token(TokenKind kind)
{
    if (!at_token(kind))
    {
        fail_at(current_token()->line, current_token()->column, "esperava '%s', encontrei '%s'",
                token_name(kind), token_name(current_token()->kind));
    }

    return &tokens->items[position++];
}

/** Erro para palavras reservadas que ainda não foram implementadas. */
/** Pula fins de linha (linhas vazias entre comandos e antes de '{' ou 'senao'). */
static void skip_newlines(void)
{
    while (accept_token(TOKEN_NEWLINE))
    {
    }
}

/** Tipo do próximo token que não é fim de linha (sem consumir nada). */
static TokenKind next_after_newlines(void)
{
    int index = position;

    while (tokens->items[index].kind == TOKEN_NEWLINE)
    {
        index++;
    }

    return tokens->items[index].kind;
}

static _Noreturn void fail_unimplemented(Token *token)
{
    fail_at(token->line, token->column, "'%s' ainda não foi implementado nesta versão",
            token_name(token->kind));
}

/** Novo nó de expressão, com a posição do token. */
static Expression *new_expression(ExpressionKind k, Token *token)
{
    Expression *expr = allocate_zeroed(sizeof *expr);

    expr->kind = k;
    expr->line = token->line;
    expr->column = token->column;
    return expr;
}

/** Novo nó de comando, com a posição do token. */
static Statement *new_statement(StatementKind k, Token *token)
{
    Statement *stmt = allocate_zeroed(sizeof *stmt);

    stmt->kind = k;
    stmt->line = token->line;
    stmt->column = token->column;
    return stmt;
}

/* ---------- expressões ---------- */

static Expression *parse_expression(void);

/* lista "a, b, c" até o fechamento (que não é consumido) */
static void parse_expression_list(ExpressionList *list, TokenKind close)
{
    if (at_token(close))
    {
        return;
    }

    do
    {
        PUSH(*list, parse_expression());
    } while (accept_token(PUNCT_COMMA));
}

/* para i=0 ate n | ate em n | thr n  →  (lo, hi, incl) */
static void parse_range_tail(Expression **lo, Expression **hi, int *incl)
{
    *lo = parse_expression();

    if (accept_token(KEYWORD_THR))
    {
        *incl = 1;
    }
    else
    {
        expect_token(KEYWORD_ATE);
        *incl = accept_token(KEYWORD_EM);
    }

    *hi = parse_expression();
}

/* gerador dentro de chamada: soma(x*x para i=0 ate n) / soma(x para x em v) */
static Expression *parse_generator(Expression *body)
{
    Expression *generator = new_expression(EXPR_GENERATOR, current_token());

    expect_token(KEYWORD_PARA);
    generator->first = body;
    generator->text = expect_token(TOKEN_NAME)->text;

    if (accept_token(KEYWORD_EM))
    {
        generator->third = parse_expression();
    }
    else
    {
        expect_token(PUNCT_ASSIGN);
        parse_range_tail(&generator->second, &generator->third, &generator->inclusive);
    }

    return generator;
}

/** Chamada f(a, b) ou gerador soma(x para i=0 ate n). */
static Expression *parse_call(Token *name_token)
{
    Expression *expr = new_expression(EXPR_CALL, name_token);

    expr->text = name_token->text;

    if (builtin_of(expr->text, lang) == BUILTIN_TAMANHO)
    {
        fail_at(name_token->line, name_token->column, "tamanho é método: escreva v.%s()",
                expr->text);
    }

    expect_token(PUNCT_LEFT_PAREN);

    if (!at_token(PUNCT_RIGHT_PAREN))
    {
        Expression *first = parse_expression();

        if (at_token(KEYWORD_PARA))
        {
            first = parse_generator(first);
        }

        PUSH(expr->arguments, first);

        while (accept_token(PUNCT_COMMA))
        {
            PUSH(expr->arguments, parse_expression());
        }
    }

    expect_token(PUNCT_RIGHT_PAREN);
    return expr;
}

/** Expressão básica: número, texto, nome, chamada, (expr), tupla, {vetor}, mapa {}. */
static Expression *parse_primary(void)
{
    Token *token = current_token();
    Expression *expr;

    switch (token->kind)
    {
    case TOKEN_INT:
        position++;
        expr = new_expression(EXPR_INT, token);
        expr->int_value = token->int_value;
        return expr;

    case TOKEN_REAL:
        position++;
        expr = new_expression(EXPR_REAL, token);
        expr->real_value = token->real_value;
        return expr;

    case TOKEN_CHAR:
        position++;
        expr = new_expression(EXPR_CHAR, token);
        expr->int_value = token->int_value;
        return expr;

    case TOKEN_STRING:
        position++;
        expr = new_expression(EXPR_STRING, token);
        expr->text = token->text;
        return expr;

    case KEYWORD_VERDADEIRO:
    case KEYWORD_FALSO:
        position++;
        expr = new_expression(EXPR_INT, token);
        expr->int_value = token->kind == KEYWORD_VERDADEIRO;
        return expr;

    case PUNCT_LEFT_PAREN:
        position++;
        expr = parse_expression();

        /* (a, b, ...) é tupla; (a) é só agrupamento */
        if (at_token(PUNCT_COMMA))
        {
            Expression *tuple = new_expression(EXPR_TUPLE, token);
            PUSH(tuple->arguments, expr);

            while (accept_token(PUNCT_COMMA))
            {
                PUSH(tuple->arguments, parse_expression());
            }

            expr = tuple;
        }

        expect_token(PUNCT_RIGHT_PAREN);
        return expr;

    case PUNCT_LEFT_BRACE:
        position++;
        expr = new_expression(EXPR_VECTOR, token);
        skip_newlines();

        while (!at_token(PUNCT_RIGHT_BRACE))
        {
            PUSH(expr->arguments, parse_expression());
            skip_newlines();

            if (!accept_token(PUNCT_COMMA))
            {
                break;
            }

            skip_newlines();
        }

        expect_token(PUNCT_RIGHT_BRACE);
        return expr;

    case TOKEN_NAME:
        position++;

        if (at_token(PUNCT_LEFT_PAREN))
        {
            return parse_call(token);
        }

        /* mapa {} */
        if (at_token(PUNCT_LEFT_BRACE) && builtin_of(token->text, lang) == BUILTIN_MAPA)
        {
            position++;
            expect_token(PUNCT_RIGHT_BRACE);
            return new_expression(EXPR_MAP, token);
        }

        expr = new_expression(EXPR_NAME, token);
        expr->text = token->text;
        return expr;

    case KEYWORD_CONJ:
    case KEYWORD_TODO:
    case KEYWORD_EXISTE:
    case KEYWORD_ALGUM:
        fail_unimplemented(token);

    default:
        fail_at(token->line, token->column, "esperava uma expressão, encontrei '%s'",
                token_name(token->kind));
    }
}

/* a[i], a[lo..hi], a.metodo(args) */
static Expression *parse_postfix(void)
{
    Expression *expr = parse_primary();

    for (;;)
    {
        Token *token = current_token();

        if (accept_token(PUNCT_LEFT_BRACKET))
        {
            Expression *first = parse_expression();
            Expression *item;

            if (accept_token(PUNCT_RANGE))
            {
                item = new_expression(EXPR_SLICE, token);
                item->third = parse_expression();
            }
            else
            {
                item = new_expression(EXPR_INDEX, token);
            }

            item->first = expr;
            item->second = first;
            expr = item;
            expect_token(PUNCT_RIGHT_BRACKET);
        }
        else if (accept_token(PUNCT_DOT))
        {
            Token *name_token = expect_token(TOKEN_NAME);
            Expression *method_call = new_expression(EXPR_METHOD, name_token);

            method_call->first = expr;
            method_call->text = name_token->text;
            expect_token(PUNCT_LEFT_PAREN);
            parse_expression_list(&method_call->arguments, PUNCT_RIGHT_PAREN);
            expect_token(PUNCT_RIGHT_PAREN);

            /* v.size() vira a chamada nativa de tamanho; dsu.size(x) tem argumento e segue método */
            if (method_call->arguments.count == 0 &&
                builtin_of(method_call->text, lang) == BUILTIN_TAMANHO)
            {
                method_call->kind = EXPR_CALL;
                PUSH(method_call->arguments, expr);
                method_call->first = NULL;
            }

            expr = method_call;
        }
        else
        {
            return expr;
        }
    }
}

/** Menos e ~ na frente de uma expressão. */
static Expression *parse_unary(void)
{
    Token *token = current_token();

    if (at_token(PUNCT_MINUS) || at_token(PUNCT_TILDE))
    {
        position++;

        Expression *expr = new_expression(EXPR_UNARY, token);
        expr->operation = token->kind;
        expr->first = parse_unary();
        return expr;
    }

    return parse_postfix();
}

/*
 * Níveis de precedência, do mais fraco para o mais forte.
 * Como em Python, bit a bit liga mais forte que comparação: "x & 1 == 0" funciona.
 * O nível 2 é o "nao", tratado à parte.
 */
static const TokenKind PRECEDENCE_LEVELS[][7] = {
    {KEYWORD_OU},
    {KEYWORD_E},
    {0},
    {PUNCT_EQUAL, PUNCT_NOT_EQUAL, PUNCT_LESS, PUNCT_LESS_EQUAL, PUNCT_GREATER, PUNCT_GREATER_EQUAL,
     KEYWORD_EM},
    {PUNCT_BAR},
    {PUNCT_CARET},
    {PUNCT_AMPERSAND},
    {PUNCT_SHIFT_LEFT, PUNCT_SHIFT_RIGHT},
    {PUNCT_PLUS, PUNCT_MINUS},
    {PUNCT_STAR, PUNCT_SLASH, PUNCT_PERCENT},
};

#define PRECEDENCE_LEVEL_COUNT ((int)(sizeof PRECEDENCE_LEVELS / sizeof *PRECEDENCE_LEVELS))
#define LEVEL_NOT              2
#define LEVEL_COMPARISON       3

/** Diz se o token é um operador de comparação (inclui em). */
static int is_comparison(TokenKind kind)
{
    return (kind >= PUNCT_EQUAL && kind <= PUNCT_GREATER_EQUAL) || kind == KEYWORD_EM;
}

/** Diz se o token é um operador do nível de precedência dado. */
static int token_in_level(int level, TokenKind kind)
{
    for (int i = 0; i < 7 && PRECEDENCE_LEVELS[level][i]; i++)
    {
        if (kind == PRECEDENCE_LEVELS[level][i])
        {
            return 1;
        }
    }

    return 0;
}

/** Expressão com operadores binários, do nível de precedência dado para cima. */
static Expression *parse_binary(int level)
{
    if (level == PRECEDENCE_LEVEL_COUNT)
    {
        return parse_unary();
    }

    if (level == LEVEL_NOT)
    {
        Token *token = current_token();

        if (!accept_token(KEYWORD_NAO))
        {
            return parse_binary(level + 1);
        }

        Expression *expr = new_expression(EXPR_UNARY, token);
        expr->operation = KEYWORD_NAO;
        expr->first = parse_binary(level);
        return expr;
    }

    Expression *left = parse_binary(level + 1);

    while (token_in_level(level, current_token()->kind))
    {
        Token *token = current_token();
        position++;

        Expression *expr = new_expression(EXPR_BINARY, token);
        expr->operation = token->kind;
        expr->first = left;
        expr->second = parse_binary(level + 1);
        left = expr;

        if (level == LEVEL_COMPARISON && is_comparison(current_token()->kind))
        {
            fail_at(current_token()->line, current_token()->column,
                    "comparação encadeada não é suportada (use 'a < b e b < c')");
        }
    }

    return left;
}

/** Uma expressão completa. */
static Expression *parse_expression(void)
{
    if (at_token(KEYWORD_UNIAO) || at_token(KEYWORD_INTER) || at_token(KEYWORD_MENOS))
    {
        fail_unimplemented(current_token());
    }

    return parse_binary(0);
}

/* ---------- comandos ---------- */

static void parse_statement(StatementList *out);

/* { comandos } (chave na mesma linha ou na seguinte), ou um único comando sem chaves */
static void parse_block(StatementList *out)
{
    skip_newlines();

    if (at_token(PUNCT_COLON))
    {
        fail_at(current_token()->line, current_token()->column,
                "blocos usam chaves: if x { ... } (o ':' era a sintaxe antiga)");
    }

    /* como em C: sem chaves, o corpo é um único comando (que pode ser outro if/for) */
    if (!at_token(PUNCT_LEFT_BRACE))
    {
        if (at_token(TOKEN_EOF) || at_token(PUNCT_RIGHT_BRACE))
        {
            fail_at(current_token()->line, current_token()->column, "falta o corpo do bloco");
        }

        parse_statement(out);
        return;
    }

    expect_token(PUNCT_LEFT_BRACE);
    skip_newlines();

    while (!accept_token(PUNCT_RIGHT_BRACE))
    {
        if (at_token(TOKEN_EOF))
        {
            fail_at(current_token()->line, current_token()->column,
                    "falta '}' para fechar o bloco");
        }

        parse_statement(out);
        skip_newlines();
    }
}

/* nome[dim]...[: tipo] */
static void parse_declaration_item(Statement *stmt)
{
    Token *name_token = expect_token(TOKEN_NAME);
    Declaration declaration = {0};

    declaration.name = name_token->text;
    declaration.line = name_token->line;
    declaration.column = name_token->column;

    while (accept_token(PUNCT_LEFT_BRACKET))
    {
        PUSH(declaration.dimensions, parse_expression());
        expect_token(PUNCT_RIGHT_BRACKET);
    }

    if (accept_token(PUNCT_COLON))
    {
        declaration.type_name = expect_token(TOKEN_NAME)->text;
    }

    PUSH(stmt->declarations, declaration);
}

/* uma linha de "a; b[n]; c: real;" */
static void parse_declaration_line(Statement *stmt)
{
    do
    {
        if (at_token(TOKEN_NEWLINE) || at_token(PUNCT_RIGHT_BRACE))
        {
            break;
        }

        parse_declaration_item(stmt);
    } while (accept_token(PUNCT_SEMICOLON));

    if (!at_token(PUNCT_RIGHT_BRACE))
    {
        expect_token(TOKEN_NEWLINE);
    }
}

/* #decl a; b;   ou   #decl + bloco indentado */
static Statement *parse_declaration(Token *token)
{
    Statement *stmt = new_statement(STMT_DECL, token);

    if (next_after_newlines() != PUNCT_LEFT_BRACE)
    {
        parse_declaration_line(stmt);
        return stmt;
    }

    skip_newlines();
    expect_token(PUNCT_LEFT_BRACE);
    skip_newlines();

    while (!accept_token(PUNCT_RIGHT_BRACE))
    {
        parse_declaration_line(stmt);
        skip_newlines();
    }

    return stmt;
}

/** fn nome(parametros): bloco. */
static Statement *parse_function(Token *token)
{
    if (function_depth)
    {
        fail_at(token->line, token->column, "funções só podem ser declaradas no topo do arquivo");
    }

    Statement *stmt = new_statement(STMT_FUNCTION, token);
    stmt->text = expect_token(TOKEN_NAME)->text;
    expect_token(PUNCT_LEFT_PAREN);

    if (!at_token(PUNCT_RIGHT_PAREN))
    {
        do
        {
            PUSH(stmt->parameters, expect_token(TOKEN_NAME)->text);
        } while (accept_token(PUNCT_COMMA));
    }

    expect_token(PUNCT_RIGHT_PAREN);

    function_depth++;
    parse_block(&stmt->body);
    function_depth--;
    return stmt;
}

/* se / senao se / senao: cada "senao se" vira um se dentro do senao anterior */
static Statement *parse_if(Token *token)
{
    Statement *stmt = new_statement(STMT_IF, token);
    Statement *last_if = stmt;

    stmt->first = parse_expression();
    parse_block(&stmt->body);

    /* o senao pode vir depois de '}' na mesma linha ou na linha seguinte */
    while (next_after_newlines() == KEYWORD_SENAO)
    {
        skip_newlines();

        Token *else_token = current_token();
        position++;

        if (!accept_token(KEYWORD_SE))
        {
            parse_block(&last_if->else_body);
            break;
        }

        Statement *else_if = new_statement(STMT_IF, else_token);
        else_if->first = parse_expression();
        parse_block(&else_if->body);
        PUSH(last_if->else_body, else_if);
        last_if = else_if;
    }

    return stmt;
}

static Statement *parse_simple_statement(Token *token);
static Statement *make_step(Token *token, Token *operator_token, Expression *target);

/** Diz se o '(' atual abre um for estilo C: tem ';' antes do ')' que o fecha. */
static int c_style_for_ahead(void)
{
    int depth = 0;

    for (int i = position; i < tokens->count; i++)
    {
        TokenKind kind = tokens->items[i].kind;

        if (kind == PUNCT_LEFT_PAREN || kind == PUNCT_LEFT_BRACKET || kind == PUNCT_LEFT_BRACE)
        {
            depth++;
        }
        else if (kind == PUNCT_RIGHT_PAREN || kind == PUNCT_RIGHT_BRACKET ||
                 kind == PUNCT_RIGHT_BRACE)
        {
            if (--depth == 0)
            {
                return 0;
            }
        }
        else if ((kind == PUNCT_SEMICOLON && depth == 1) || kind == TOKEN_NEWLINE ||
                 kind == TOKEN_EOF)
        {
            return kind == PUNCT_SEMICOLON;
        }
    }

    return 0;
}

/** Passo do for estilo C: i++, ++i, i--, i += 2, i = i * 2. */
static Statement *parse_loop_step(void)
{
    Token *token = current_token();

    if (accept_token(PUNCT_INCREMENT) || accept_token(PUNCT_DECREMENT))
    {
        return make_step(token, token, parse_expression());
    }

    return parse_simple_statement(token);
}

/** Diz se a expressão pode mudar o nome: chama função do usuário ou método nele (v.add). */
static int expression_may_write(Expression *expr, const char *name)
{
    if (!expr)
    {
        return 0;
    }

    if ((expr->kind == EXPR_CALL && builtin_of(expr->text, lang) == BUILTIN_NONE) ||
        (expr->kind == EXPR_METHOD && expr->first->kind == EXPR_NAME &&
         !strcmp(expr->first->text, name)))
    {
        return 1;
    }

    for (int i = 0; i < expr->arguments.count; i++)
    {
        if (expression_may_write(expr->arguments.items[i], name))
        {
            return 1;
        }
    }

    return expression_may_write(expr->first, name) || expression_may_write(expr->second, name) ||
           expression_may_write(expr->third, name);
}

/** Diz se a expressão é o nome ou um padrão (a, b) que o contém. */
static int names_target(Expression *expr, const char *name)
{
    if (expr->kind == EXPR_NAME)
    {
        return !strcmp(expr->text, name);
    }

    for (int i = 0; expr->kind == EXPR_TUPLE && i < expr->arguments.count; i++)
    {
        if (names_target(expr->arguments.items[i], name))
        {
            return 1;
        }
    }

    return 0;
}

/** Diz se o ler(...) escreve no nome: ler(n), ler(v[i]) e ler(v[0..k]) escrevem em n e v. */
static int read_writes(Expression *call, const char *name)
{
    for (int i = 0; i < call->arguments.count; i++)
    {
        Expression *target = call->arguments.items[i];

        while (target->kind == EXPR_INDEX || target->kind == EXPR_SLICE)
        {
            target = target->first;
        }

        if (names_target(target, name))
        {
            return 1;
        }
    }

    return 0;
}

/** Diz se algum comando pode mudar o nome (atribuição, leitura, laço, função ou método). */
static int statements_may_write(StatementList *statements, const char *name)
{
    for (int i = 0; i < statements->count; i++)
    {
        Statement *stmt = statements->items[i];
        Builtin builtin = stmt->kind == STMT_EXPR && stmt->first->kind == EXPR_CALL
                              ? builtin_of(stmt->first->text, lang)
                              : BUILTIN_NONE;

        if (stmt->kind == STMT_DECL || stmt->kind == STMT_FUNCTION ||
            (stmt->kind == STMT_ASSIGN && names_target(stmt->first, name)) ||
            ((builtin == BUILTIN_LER || builtin == BUILTIN_LER_LINHA) &&
             read_writes(stmt->first, name)) ||
            (stmt->text && !strcmp(stmt->text, name)))
        {
            return 1;
        }

        for (int j = 0; j < stmt->parameters.count; j++)
        {
            if (!strcmp(stmt->parameters.items[j], name))
            {
                return 1;
            }
        }

        if (expression_may_write(stmt->first, name) || expression_may_write(stmt->second, name) ||
            expression_may_write(stmt->third, name) || statements_may_write(&stmt->body, name) ||
            statements_may_write(&stmt->else_body, name))
        {
            return 1;
        }
    }

    return 0;
}

/** Diz se o limite do for não muda no corpo: números, nomes, contas e v.size(). */
static int bound_is_invariant(Expression *bound, StatementList *body)
{
    switch (bound->kind)
    {
    case EXPR_INT:
        return 1;

    case EXPR_NAME:
        return !statements_may_write(body, bound->text);

    case EXPR_BINARY:
        return bound_is_invariant(bound->first, body) && bound_is_invariant(bound->second, body);

    case EXPR_CALL:
        return builtin_of(bound->text, lang) == BUILTIN_TAMANHO &&
               bound_is_invariant(bound->arguments.items[0], body);

    default:
        return 0;
    }
}

/**
 * for (i = a; i < b; i++) com i e b fixos no corpo vira o for nativo (i=a to b), que a
 * análise de faixa entende; devolve NULL se o laço não tem essa forma.
 */
static Statement *as_counting_for(Token *token, Statement *init, Expression *condition,
                                  Statement *step, StatementList *body)
{
    if (!init || !step || init->kind != STMT_ASSIGN || init->first->kind != EXPR_NAME ||
        condition->kind != EXPR_BINARY ||
        (condition->operation != PUNCT_LESS && condition->operation != PUNCT_LESS_EQUAL) ||
        !names_target(condition->first, init->first->text) || step->kind != STMT_ASSIGN ||
        !names_target(step->first, init->first->text) || step->second->operation != PUNCT_PLUS ||
        step->second->second->kind != EXPR_INT || step->second->second->int_value != 1)
    {
        return NULL;
    }

    const char *name = init->first->text;

    if (expression_may_write(init->second, name) || statements_may_write(body, name) ||
        !bound_is_invariant(condition->second, body))
    {
        return NULL;
    }

    Statement *loop = new_statement(STMT_FOR, token);
    loop->text = init->first->text;
    loop->first = init->second;
    loop->second = condition->second;
    loop->inclusive = condition->operation == PUNCT_LESS_EQUAL;
    loop->body = *body;
    return loop;
}

/**
 * for (início; condição; passo) como no C: o início vira um comando antes do laço,
 * e o laço é um while cujo else_body guarda o passo (roda também no continue).
 */
static void parse_c_style_for(Token *token, StatementList *out)
{
    Statement *init = NULL;
    Statement *step = NULL;
    Expression *condition;

    if (!at_token(PUNCT_SEMICOLON))
    {
        init = parse_simple_statement(current_token());
    }

    expect_token(PUNCT_SEMICOLON);

    if (at_token(PUNCT_SEMICOLON))
    {
        condition = new_expression(EXPR_INT, token);
        condition->int_value = 1;
    }
    else
    {
        condition = parse_expression();
    }

    expect_token(PUNCT_SEMICOLON);

    if (!at_token(PUNCT_RIGHT_PAREN))
    {
        step = parse_loop_step();
    }

    expect_token(PUNCT_RIGHT_PAREN);

    StatementList body = {0};
    parse_block(&body);

    Statement *counting = as_counting_for(token, init, condition, step, &body);

    if (counting)
    {
        PUSH(*out, counting);
        return;
    }

    Statement *loop = new_statement(STMT_WHILE, token);
    loop->first = condition;
    loop->body = body;

    if (step)
    {
        PUSH(loop->else_body, step);
    }

    if (init)
    {
        PUSH(*out, init);
    }

    PUSH(*out, loop);
}

/** Nomes de (a, b) no for, depois do '(' já consumido, até o ')'. */
static void parse_for_pattern(Statement *stmt)
{
    do
    {
        PUSH(stmt->parameters, expect_token(TOKEN_NAME)->text);
    } while (accept_token(PUNCT_COMMA));

    expect_token(PUNCT_RIGHT_PAREN);
}

/**
 * para i=a ate b, para x em v, para (a, b) em v; com parênteses opcionais em volta
 * (for (i=0 to n), for (x in v), for ((a, b) in v)) e o for do C (for (i=0; i<n; i++)).
 */
static void parse_for(Token *token, StatementList *out)
{
    if (at_token(PUNCT_LEFT_PAREN) && c_style_for_ahead())
    {
        position++;
        parse_c_style_for(token, out);
        return;
    }

    /* for (a, b) in v: o parêntese é o padrão de tupla, não o do for */
    int tuple_outside = at_token(PUNCT_LEFT_PAREN) &&
                        tokens->items[position + 1].kind == TOKEN_NAME &&
                        tokens->items[position + 2].kind == PUNCT_COMMA;
    int parenthesized = !tuple_outside && accept_token(PUNCT_LEFT_PAREN);

    if (at_token(KEYWORD_TODO) || at_token(KEYWORD_ALGUM))
    {
        fail_unimplemented(current_token());
    }

    Statement *stmt;

    if (accept_token(PUNCT_LEFT_PAREN))
    {
        stmt = new_statement(STMT_FOR_IN, token);
        parse_for_pattern(stmt);
        expect_token(KEYWORD_EM);
        stmt->first = parse_expression();
    }
    else
    {
        Token *variable_token = expect_token(TOKEN_NAME);

        if (accept_token(KEYWORD_EM))
        {
            stmt = new_statement(STMT_FOR_IN, token);
            stmt->first = parse_expression();
        }
        else
        {
            stmt = new_statement(STMT_FOR, token);
            expect_token(PUNCT_ASSIGN);
            parse_range_tail(&stmt->first, &stmt->second, &stmt->inclusive);
        }

        stmt->text = variable_token->text;
    }

    if (parenthesized)
    {
        expect_token(PUNCT_RIGHT_PAREN);
    }

    parse_block(&stmt->body);
    PUSH(*out, stmt);
}

/* f(x) = corpo  vira  fn f(x): ret corpo */
static int is_one_line_function(Expression *lhs)
{
    if (lhs->kind != EXPR_CALL || function_depth)
    {
        return 0;
    }

    for (int i = 0; i < lhs->arguments.count; i++)
    {
        if (lhs->arguments.items[i]->kind != EXPR_NAME)
        {
            return 0;
        }
    }

    return 1;
}

/** Transforma f(x) = corpo em fn f(x): ret corpo. */
static Statement *make_one_line_function(Token *token, Expression *lhs, Expression *body)
{
    Statement *stmt = new_statement(STMT_FUNCTION, token);
    Statement *return_stmt = new_statement(STMT_RETURN, token);

    stmt->text = lhs->text;

    for (int i = 0; i < lhs->arguments.count; i++)
    {
        PUSH(stmt->parameters, lhs->arguments.items[i]->text);
    }

    return_stmt->first = body;
    PUSH(stmt->body, return_stmt);
    return stmt;
}

/* atribuição, função de uma linha ou expressão solta */
/** Operador binário de uma atribuição composta (+= vira +); 0 se não for uma. */
static TokenKind compound_operator(TokenKind kind)
{
    switch (kind)
    {
    case PUNCT_PLUS_ASSIGN:
        return PUNCT_PLUS;

    case PUNCT_MINUS_ASSIGN:
        return PUNCT_MINUS;

    case PUNCT_STAR_ASSIGN:
        return PUNCT_STAR;

    case PUNCT_SLASH_ASSIGN:
        return PUNCT_SLASH;

    case PUNCT_PERCENT_ASSIGN:
        return PUNCT_PERCENT;

    case PUNCT_AMPERSAND_ASSIGN:
        return PUNCT_AMPERSAND;

    case PUNCT_BAR_ASSIGN:
        return PUNCT_BAR;

    case PUNCT_CARET_ASSIGN:
        return PUNCT_CARET;

    case PUNCT_SHIFT_LEFT_ASSIGN:
        return PUNCT_SHIFT_LEFT;

    case PUNCT_SHIFT_RIGHT_ASSIGN:
        return PUNCT_SHIFT_RIGHT;

    default:
        return 0;
    }
}

/**
 * Monta "alvo = alvo op quantidade". O alvo é o mesmo nó nos dois lados:
 * é lido e escrito, sem cópia.
 */
static Statement *make_update(Token *token, Token *operator_token, TokenKind operation,
                              Expression *target, Expression *amount)
{
    if (target->kind != EXPR_NAME && target->kind != EXPR_INDEX)
    {
        fail_at(operator_token->line, operator_token->column,
                "'%s' só funciona com variável ou posição de vetor",
                token_name(operator_token->kind));
    }

    Expression *value = new_expression(EXPR_BINARY, operator_token);
    value->operation = operation;
    value->first = target;
    value->second = amount;

    Statement *stmt = new_statement(STMT_ASSIGN, token);
    stmt->first = target;
    stmt->second = value;
    return stmt;
}

/** x += e, x -= e, ... (iguais ao C) viram x = x op e. */
static Statement *make_compound_assignment(Token *token, Expression *target)
{
    Token *operator_token = current_token();
    TokenKind operation = compound_operator(operator_token->kind);

    position++;
    return make_update(token, operator_token, operation, target, parse_expression());
}

/** x++ / ++x / x-- / --x: soma ou subtrai 1 (só como comando, como o x += 1). */
static Statement *make_step(Token *token, Token *operator_token, Expression *target)
{
    Expression *one = new_expression(EXPR_INT, operator_token);

    one->int_value = 1;
    return make_update(token, operator_token,
                       operator_token->kind == PUNCT_INCREMENT ? PUNCT_PLUS : PUNCT_MINUS, target,
                       one);
}

/** Atribuição (inclusive composta e a tupla), função de uma linha ou expressão solta. */
static Statement *parse_simple_statement(Token *token)
{
    /* quem escreveu a palavra antiga de função recebe a dica certa */
    if (token->kind == TOKEN_NAME &&
        (!strcmp(token->text, "funcao") || !strcmp(token->text, "function")) &&
        tokens->items[position + 1].kind == TOKEN_NAME)
    {
        fail_at(token->line, token->column, "função se declara com 'fn': fn nome(a, b):");
    }

    if (token->kind == TOKEN_NAME && (!strcmp(token->text, "inc") || !strcmp(token->text, "dec")) &&
        tokens->items[position + 1].kind == TOKEN_NAME)
    {
        fail_at(token->line, token->column, "'%s x' agora é x%s", token->text,
                token->text[0] == 'i' ? "++" : "--");
    }

    Expression *expr = parse_expression();

    if (compound_operator(current_token()->kind))
    {
        return make_compound_assignment(token, expr);
    }

    if (at_token(PUNCT_INCREMENT) || at_token(PUNCT_DECREMENT))
    {
        Token *operator_token = current_token();

        position++;
        return make_step(token, operator_token, expr);
    }

    if (!accept_token(PUNCT_ASSIGN))
    {
        Statement *stmt = new_statement(STMT_EXPR, token);
        stmt->first = expr;
        return stmt;
    }

    Expression *value = parse_expression();

    if (is_one_line_function(expr))
    {
        return make_one_line_function(token, expr, value);
    }

    int valid = expr->kind == EXPR_NAME || expr->kind == EXPR_INDEX;

    /* (a, b) = ... desmonta a tupla */
    if (expr->kind == EXPR_TUPLE)
    {
        valid = 1;

        for (int i = 0; i < expr->arguments.count; i++)
        {
            Expression *item = expr->arguments.items[i];
            valid &= item->kind == EXPR_NAME || item->kind == EXPR_INDEX;
        }
    }

    if (!valid)
    {
        fail_at(expr->line, expr->column,
                "só dá para atribuir a variável, posição de vetor ou (a, b)");
    }

    Statement *stmt = new_statement(STMT_ASSIGN, token);
    stmt->first = expr;
    stmt->second = value;
    return stmt;
}

/** Um comando e o fim de linha dele. */
/** Fim de comando: um ';' opcional (para quem está acostumado com C) e o fim da linha. */
static void end_statement(void)
{
    accept_token(PUNCT_SEMICOLON);

    /* um comando pode terminar logo antes do '}' (if x { y = 1 }) ou do else (if (x) a else b) */
    if (!at_token(PUNCT_RIGHT_BRACE) && !at_token(KEYWORD_SENAO))
    {
        expect_token(TOKEN_NEWLINE);
    }
}

static void parse_statement(StatementList *out)
{
    Token *token = current_token();
    Statement *stmt;

    switch (token->kind)
    {
    case KEYWORD_DECL:
        position++;
        PUSH(*out, parse_declaration(token));
        return;

    case KEYWORD_FUNCAO:
        position++;
        PUSH(*out, parse_function(token));
        return;

    case KEYWORD_SE:
        position++;
        PUSH(*out, parse_if(token));
        return;

    case KEYWORD_ENQUANTO:
        position++;
        stmt = new_statement(STMT_WHILE, token);
        stmt->first = parse_expression();
        parse_block(&stmt->body);
        PUSH(*out, stmt);
        return;

    case KEYWORD_PARA:
        position++;
        parse_for(token, out);
        return;

    case KEYWORD_RET:
        position++;
        stmt = new_statement(STMT_RETURN, token);

        if (!at_token(TOKEN_NEWLINE))
        {
            stmt->first = parse_expression();
        }

        end_statement();
        PUSH(*out, stmt);
        return;

    case PUNCT_INCREMENT:
    case PUNCT_DECREMENT:
        position++;
        PUSH(*out, make_step(token, token, parse_expression()));
        end_statement();
        return;

    case KEYWORD_PARE:
    case KEYWORD_CONTINUE:
        position++;
        PUSH(*out, new_statement(token->kind == KEYWORD_PARE ? STMT_BREAK : STMT_CONTINUE, token));
        end_statement();
        return;

    case KEYWORD_TIPO:
        fail_unimplemented(token);

    default:
        PUSH(*out, parse_simple_statement(token));
        end_statement();
        return;
    }
}

/** Lê o programa inteiro (lista de tokens) e devolve a lista de comandos. */
StatementList parse(TokenList *t, Language language)
{
    StatementList out = {0};

    tokens = t;
    lang = language;
    position = 0;
    function_depth = 0;

    skip_newlines();

    while (!at_token(TOKEN_EOF))
    {
        parse_statement(&out);
        skip_newlines();
    }

    return out;
}
