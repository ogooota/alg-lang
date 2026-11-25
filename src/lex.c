#include "alg.h"

typedef struct
{
    const char *text;
    TokenKind kind;
} Spelling;

/* símbolos matemáticos opcionais */
static const Spelling MATH_SYMBOLS[] = {
    {"≤", PUNCT_LESS_EQUAL}, {"≥", PUNCT_GREATER_EQUAL}, {"≠", PUNCT_NOT_EQUAL},
    {"←", PUNCT_ASSIGN},     {"∈", KEYWORD_EM},          {"∪", KEYWORD_UNIAO},
    {"∩", KEYWORD_INTER},    {"∀", KEYWORD_TODO},        {"∃", KEYWORD_EXISTE},
};

/* operadores com mais de um caractere vêm primeiro */
static const Spelling OPERATORS[] = {
    {"<<=", PUNCT_SHIFT_LEFT_ASSIGN},
    {">>=", PUNCT_SHIFT_RIGHT_ASSIGN},
    {"++", PUNCT_INCREMENT},
    {"--", PUNCT_DECREMENT},
    {"+=", PUNCT_PLUS_ASSIGN},
    {"->", PUNCT_ARROW},
    {"-=", PUNCT_MINUS_ASSIGN},
    {"*=", PUNCT_STAR_ASSIGN},
    {"/=", PUNCT_SLASH_ASSIGN},
    {"%=", PUNCT_PERCENT_ASSIGN},
    {"&=", PUNCT_AMPERSAND_ASSIGN},
    {"|=", PUNCT_BAR_ASSIGN},
    {"^=", PUNCT_CARET_ASSIGN},
    {"..", PUNCT_RANGE},
    {"==", PUNCT_EQUAL},
    {"!=", PUNCT_NOT_EQUAL},
    {"<=", PUNCT_LESS_EQUAL},
    {">=", PUNCT_GREATER_EQUAL},
    {"<<", PUNCT_SHIFT_LEFT},
    {">>", PUNCT_SHIFT_RIGHT},
    {"(", PUNCT_LEFT_PAREN},
    {")", PUNCT_RIGHT_PAREN},
    {"[", PUNCT_LEFT_BRACKET},
    {"]", PUNCT_RIGHT_BRACKET},
    {"{", PUNCT_LEFT_BRACE},
    {"}", PUNCT_RIGHT_BRACE},
    {",", PUNCT_COMMA},
    {":", PUNCT_COLON},
    {";", PUNCT_SEMICOLON},
    {".", PUNCT_DOT},
    {"=", PUNCT_ASSIGN},
    {"<", PUNCT_LESS},
    {">", PUNCT_GREATER},
    {"+", PUNCT_PLUS},
    {"-", PUNCT_MINUS},
    {"*", PUNCT_STAR},
    {"/", PUNCT_SLASH},
    {"%", PUNCT_PERCENT},
    {"&", PUNCT_AMPERSAND},
    {"|", PUNCT_BAR},
    {"^", PUNCT_CARET},
    {"~", PUNCT_TILDE},
};

#define COUNT(a) (sizeof(a) / sizeof *(a))

/** Nome legível de um tipo de token, para mensagens de erro. */
const char *token_name(TokenKind kind)
{
    switch (kind)
    {
    case TOKEN_EOF:
        return "fim do arquivo";

    case TOKEN_NEWLINE:
        return "fim de linha";

    case TOKEN_INDENT:
        return "indentação";

    case TOKEN_DEDENT:
        return "fim de bloco";

    case TOKEN_INT:
    case TOKEN_REAL:
        return "número";

    case TOKEN_CHAR:
        return "caractere";

    case TOKEN_STRING:
        return "texto";

    case TOKEN_NAME:
        return "nome";

    case KEYWORD_DECL:
        return "#decl";

    default:
        break;
    }

    if (kind >= KEYWORD_SE && kind <= KEYWORD_TIPO)
    {
        return keyword_spelling(kind);
    }

    for (size_t i = 0; i < COUNT(OPERATORS); i++)
    {
        if (OPERATORS[i].kind == kind)
        {
            return OPERATORS[i].text;
        }
    }

    return "?";
}

/* ---------- estado do lexer ---------- */

typedef struct
{
    TokenList tokens;
    Language language;
    const char *cursor;     /* posição atual */
    const char *line_start; /* para calcular a coluna */
    int line;
    int paren_depth;   /* parênteses abertos: dentro deles, quebra de linha não conta */
    int at_line_start; /* no começo de uma linha lógica */
    int indent_stack[256];
    int indent_depth;
} Lexer;

/** Coluna (a partir de 1) da posição atual do lexer. */
static int column_of(Lexer *lexer)
{
    return (int)(lexer->cursor - lexer->line_start) + 1;
}

/** Acrescenta um token sem valor (palavra, símbolo, INDENT...) à lista. */
static void emit_token(Lexer *lexer, TokenKind kind, int line, int column)
{
    Token token = {0};

    token.kind = kind;
    token.line = line;
    token.column = column;
    PUSH(lexer->tokens, token);
}

/** Pula o fim de linha e atualiza o número da linha. */
static void next_line(Lexer *lexer)
{
    lexer->cursor++;
    lexer->line++;
    lexer->line_start = lexer->cursor;
}

/** Diz se o caractere é um dígito decimal. */
static int is_digit(unsigned char character)
{
    return character >= '0' && character <= '9';
}

/** Diz se o caractere pode começar um nome (letra, _ ou byte UTF-8). */
static int is_ident_start(unsigned char character)
{
    return character == '_' || (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') || character >= 0x80;
}

/** Diz se o caractere pode continuar um nome. */
static int is_ident_char(unsigned char character)
{
    return is_ident_start(character) || is_digit(character);
}

/* ---------- partes do lexer ---------- */

/* primeira linha: #lang pt | #lang en */
static void lex_language_line(Lexer *lexer)
{
    if (strncmp(lexer->cursor, "#lang", 5))
    {
        fail_at(lexer->line, 1, "a primeira linha deve ser '#lang pt' ou '#lang en'");
    }

    lexer->cursor += 5;

    while (*lexer->cursor == ' ')
    {
        lexer->cursor++;
    }

    if (!strncmp(lexer->cursor, "pt", 2))
    {
        lexer->language = LANG_PT;
    }
    else if (!strncmp(lexer->cursor, "en", 2))
    {
        lexer->language = LANG_EN;
    }
    else
    {
        fail_at(lexer->line, column_of(lexer), "idioma desconhecido (use pt ou en)");
    }

    lexer->cursor += 2;

    while (*lexer->cursor == ' ' || *lexer->cursor == '\r')
    {
        lexer->cursor++;
    }

    if (*lexer->cursor && *lexer->cursor != '\n')
    {
        fail_at(lexer->line, column_of(lexer), "texto sobrando depois de #lang");
    }

    if (*lexer->cursor)
    {
        next_line(lexer);
    }
}

/*
 * Começo de linha: mede a indentação e emite INDENT/DEDENT.
 * Devolve 0 se a linha for vazia ou só comentário (e já pula ela).
 */
static int lex_indentation(Lexer *lexer, int has_lang)
{
    int width = 0;
    const char *scan = lexer->cursor;

    for (; *scan == ' ' || *scan == '\t'; scan++)
    {
        width += *scan == '\t' ? 4 - width % 4 : 1;
    }

    int is_blank = *scan == '\n' || *scan == '\r' || !*scan || (scan[0] == '/' && scan[1] == '/');

    if (is_blank)
    {
        while (*scan && *scan != '\n')
        {
            scan++;
        }

        lexer->cursor = scan;

        if (*scan)
        {
            next_line(lexer);
        }

        return 0;
    }

    lexer->cursor = scan;
    lexer->at_line_start = 0;

    if (!has_lang)
    {
        return 1;
    }

    return 1;
}

/** Lê um número inteiro (64 bits, com checagem de estouro) ou real. */
static void lex_number(Lexer *lexer)
{
    Token token = {0};
    const char *scan = lexer->cursor;
    int has_real = 0;

    token.line = lexer->line;
    token.column = column_of(lexer);

    while (is_digit((unsigned char)*scan))
    {
        scan++;
    }

    /* "0..n" é inteiro seguido de "..", então o real exige dígito depois do ponto */
    if (*scan == '.' && is_digit((unsigned char)scan[1]))
    {
        has_real = 1;
        scan++;

        while (is_digit((unsigned char)*scan))
        {
            scan++;
        }
    }

    if (has_real)
    {
        token.kind = TOKEN_REAL;
        token.real_value = strtod(lexer->cursor, NULL);
    }
    else
    {
        uint64_t value = 0;

        for (const char *copy = lexer->cursor; copy < scan; copy++)
        {
            uint64_t digit = (uint64_t)(*copy - '0');

            if (value > ((uint64_t)INT64_MAX - digit) / 10)
            {
                fail_at(token.line, token.column, "número grande demais para 64 bits");
            }

            value = value * 10 + digit;
        }

        token.kind = TOKEN_INT;
        token.int_value = (int64_t)value;
    }

    PUSH(lexer->tokens, token);
    lexer->cursor = scan;
}

/** Lê um nome; se for palavra-chave do idioma do arquivo, vira o token dela. */
static void lex_name(Lexer *lexer)
{
    Token token = {0};
    const char *scan = lexer->cursor;

    while (is_ident_char((unsigned char)*scan))
    {
        scan++;
    }

    token.kind = TOKEN_NAME;
    token.line = lexer->line;
    token.column = column_of(lexer);
    token.text = allocate_zeroed((size_t)(scan - lexer->cursor) + 1);
    memcpy(token.text, lexer->cursor, (size_t)(scan - lexer->cursor));

    token.kind = keyword_of(token.text, lexer->language);

    PUSH(lexer->tokens, token);
    lexer->cursor = scan;
}

/** Traduz o caractere depois da barra em \n, \t, \" ou \\ (0 se não existe). */
static char unescape(char character)
{
    switch (character)
    {
    case 'n':
        return '\n';

    case 't':
        return '\t';

    case '"':
        return '"';

    case '\'':
        return '\'';

    case '\\':
        return '\\';

    default:
        return 0;
    }
}

/** Lê um texto entre aspas, traduzindo os escapes. */
/** Caractere entre aspas simples: 'a', '\\n', '\\''. Vira um token com o código do byte. */
static void lex_char(Lexer *lexer)
{
    Token token = {0};

    token.kind = TOKEN_CHAR;
    token.line = lexer->line;
    token.column = column_of(lexer);
    lexer->cursor++;

    unsigned char value = (unsigned char)*lexer->cursor;

    if (value == '\\')
    {
        lexer->cursor++;
        value = (unsigned char)unescape(*lexer->cursor);

        if (!value)
        {
            fail_at(token.line, token.column,
                    "escape desconhecido em caractere (use \\n \\t \\' \\\\)");
        }
    }
    else if (value == '\'' || value == '\n' || !value)
    {
        fail_at(token.line, token.column, "caractere vazio: escreva algo entre as aspas, como 'a'");
    }

    lexer->cursor++;

    if (*lexer->cursor != '\'')
    {
        fail_at(token.line, token.column,
                "caractere tem uma letra só; para texto use aspas duplas");
    }

    lexer->cursor++;
    token.int_value = value;
    PUSH(lexer->tokens, token);
}

static void lex_string(Lexer *lexer)
{
    Token token = {0};
    TextBuffer out = {0};

    token.kind = TOKEN_STRING;
    token.line = lexer->line;
    token.column = column_of(lexer);
    lexer->cursor++;

    while (*lexer->cursor != '"')
    {
        if (!*lexer->cursor || *lexer->cursor == '\n')
        {
            fail_at(token.line, token.column, "texto sem aspas de fechamento");
        }

        char character = *lexer->cursor++;

        if (character == '\\')
        {
            character = unescape(*lexer->cursor++);

            if (!character)
            {
                fail_at(lexer->line, column_of(lexer) - 1,
                        "escape desconhecido (use \\n \\t \\\" \\\\)");
            }
        }

        char single[2] = {character, 0};
        text_append(&out, single);
    }

    lexer->cursor++;
    token.text = out.text ? out.text : duplicate_string("");
    PUSH(lexer->tokens, token);
}

/** Lê #decl; qualquer outra diretiva (fora a primeira linha #lang) é erro. */
static void lex_directive(Lexer *lexer)
{
    if (!strncmp(lexer->cursor, "#decl", 5) && !is_ident_char((unsigned char)lexer->cursor[5]))
    {
        emit_token(lexer, KEYWORD_DECL, lexer->line, column_of(lexer));
        lexer->cursor += 5;
        return;
    }

    if (!strncmp(lexer->cursor, "#lang", 5))
    {
        fail_at(lexer->line, column_of(lexer), "#lang só pode aparecer na primeira linha");
    }

    fail_at(lexer->line, column_of(lexer),
            "diretiva desconhecida (as diretivas são #lang e #decl; comentário é //)");
}

/* tenta casar um símbolo da tabela; devolve 1 se casou */
static int lex_symbol(Lexer *lexer, const Spelling *table, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        size_t length = strlen(table[i].text);

        if (strncmp(lexer->cursor, table[i].text, length))
        {
            continue;
        }

        TokenKind kind = table[i].kind;

        if (kind == PUNCT_LEFT_PAREN || kind == PUNCT_LEFT_BRACKET)
        {
            lexer->paren_depth++;
        }

        if ((kind == PUNCT_RIGHT_PAREN || kind == PUNCT_RIGHT_BRACKET) && lexer->paren_depth > 0)
        {
            lexer->paren_depth--;
        }

        emit_token(lexer, kind, lexer->line, column_of(lexer));
        lexer->cursor += length;
        return 1;
    }

    return 0;
}

/* ---------- laço principal ---------- */

TokenList tokenize(const char *source, Language *language)
{
    Lexer lexer = {0};
    int has_lang = 0;

    lexer.cursor = source;
    lexer.line_start = source;
    lexer.line = 1;
    lexer.at_line_start = 1;
    lexer.indent_depth = 1;

    while (*lexer.cursor)
    {
        if (lexer.at_line_start && lexer.paren_depth == 0)
        {
            if (!lex_indentation(&lexer, has_lang))
            {
                continue;
            }

            if (!has_lang)
            {
                lex_language_line(&lexer);
                has_lang = 1;
                lexer.at_line_start = 1;
                continue;
            }
        }

        unsigned char character = (unsigned char)*lexer.cursor;

        if (character == '\n')
        {
            int last_is_nl = lexer.tokens.count &&
                             lexer.tokens.items[lexer.tokens.count - 1].kind == TOKEN_NEWLINE;

            if (lexer.paren_depth == 0 && lexer.tokens.count && !last_is_nl)
            {
                emit_token(&lexer, TOKEN_NEWLINE, lexer.line, column_of(&lexer));
            }

            next_line(&lexer);
            lexer.at_line_start = lexer.paren_depth == 0;
        }
        else if (character == ' ' || character == '\t' || character == '\r')
        {
            lexer.cursor++;
        }
        else if (lexer.cursor[0] == '/' && lexer.cursor[1] == '/')
        {
            while (*lexer.cursor && *lexer.cursor != '\n')
            {
                lexer.cursor++;
            }
        }
        else if (character == '#')
        {
            lex_directive(&lexer);
        }
        else if (lex_symbol(&lexer, MATH_SYMBOLS, COUNT(MATH_SYMBOLS)))
        {
            continue;
        }
        else if (is_digit(character))
        {
            lex_number(&lexer);
        }
        else if (is_ident_start(character))
        {
            lex_name(&lexer);
        }
        else if (character == '"')
        {
            lex_string(&lexer);
        }
        else if (character == '\'')
        {
            lex_char(&lexer);
        }
        else if (!lex_symbol(&lexer, OPERATORS, COUNT(OPERATORS)))
        {
            fail_at(lexer.line, column_of(&lexer), "caractere inesperado '%c'", character);
        }
    }

    if (!has_lang)
    {
        fail_at(1, 1, "a primeira linha deve ser '#lang pt' ou '#lang en'");
    }

    if (lexer.tokens.count && lexer.tokens.items[lexer.tokens.count - 1].kind != TOKEN_NEWLINE)
    {
        emit_token(&lexer, TOKEN_NEWLINE, lexer.line, 1);
    }

    while (lexer.indent_depth > 1)
    {
        lexer.indent_depth--;
        emit_token(&lexer, TOKEN_DEDENT, lexer.line, 1);
    }

    emit_token(&lexer, TOKEN_EOF, lexer.line, 1);
    *language = lexer.language;
    return lexer.tokens;
}
