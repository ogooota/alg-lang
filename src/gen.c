#include "gen_internal.h"

#include "rt_arr.inc"
#include "rt_core.inc"
#include "rt_fenwick.inc"
#include "rt_rmq.inc"
#include "rt_segtree.inc"
#include "rt_graph.inc"
#include "rt_heap.inc"
#include "rt_map.inc"
#include "rt_queue.inc"

int temp_counter;
const ElementKind ELEMENT_KINDS[ELEMENT_KIND_COUNT] = {
    [ELEMENT_U8] = {"u8", "U8", "uint8_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                    "(int64_t)", "", 0},
    [ELEMENT_I8] = {"i8", "I8", "int8_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                    "(int64_t)", "", 0},
    [ELEMENT_U16] = {"u16", "U16", "uint16_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I16] = {"i16", "I16", "int16_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_U32] = {"u32", "U32", "uint32_t", "int64_t", "read_int", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I32] = {"i32", "I32", "int32_t", "int64_t", "read_int32", "write_int", "LESS_THAN",
                     "(int64_t)", "", 0},
    [ELEMENT_I32INF] = {"i32_inf", "I32Inf", "int32_t", "int64_t", "read_int32", "write_int",
                        "LESS_THAN", "decode_int32_inf", "", 0},
    [ELEMENT_I64] = {"i64", "I64", "int64_t", "int64_t", "read_int", "write_int", "LESS_THAN", "",
                     "", 0},
    [ELEMENT_REAL] = {"real", "Real", "double", "double", "read_real", "write_real", "LESS_THAN",
                      "", "", 1},
};
Program *generated_program;
/* templates de vetor usados (só esses entram no C gerado) */
int element_used[ELEMENT_KIND_COUNT];
/* tuplas usadas: cada formato vira uma struct e um template de vetor */
ElementKindList tuples;
TemplateUseList templates;
/* vetores do #decl já garantidos no trecho que está sendo gerado */
VariableSet ensured_lazy;
/*
 * Com -O, pede ao gcc otimizações extras de dentro do próprio arquivo:
 * assim valem também quando o juiz compila o C gerado com as flags dele.
 */
const char *PRAGMAS = "#pragma GCC optimize(\"O3,unroll-loops\")\n"
                      "#pragma GCC target(\"avx2\")\n";

/* struct da tupla e suas funções: ordem, igualdade, hash, escrita, leitura */
void emit_tuple(TextBuffer *out, const ElementKind *kind)
{
    const char *signature = kind->signature;
    const char *type = kind->c_type;
    const char *prefix = kind->suffix;
    int count = (int)strlen(signature);

    text_append(out, "\ntypedef struct\n{\n");

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    %s field_%d;\n", signature[i] == 'r' ? "double" : "int64_t", i);
    }

    text_appendf(out, "} %s;\n\n", type);

    /* uma expressão só, sem if: o gcc a compila sem desvio (cmov), e a escolha entre irmãos
     * aleatórios do heap deixa de errar a previsão de desvio (medido: 2x no heap) */
    text_appendf(out, "static inline int %s_less(%s left, %s right)\n{\n    return ", prefix, type,
                 type);

    for (int i = 0; i < count - 1; i++)
    {
        text_appendf(out, "left.field_%d < right.field_%d || (left.field_%d == right.field_%d && (",
                     i, i, i, i);
    }

    text_appendf(out, "left.field_%d < right.field_%d", count - 1, count - 1);

    for (int i = 0; i < count - 1; i++)
    {
        text_append(out, "))");
    }

    text_append(out, ";\n}\n\n");

    text_appendf(out, "static inline int %s_equal(%s left, %s right)\n{\n    return 1", prefix,
                 type, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, " && left.field_%d == right.field_%d", i, i);
    }

    text_append(out, ";\n}\n\n");

    text_appendf(out, "static inline uint64_t %s_hash(%s tuple)\n{\n    uint64_t hash = 0;\n",
                 prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    hash = mix_bits(hash ^ %s(tuple.field_%d));\n",
                     signature[i] == 'r' ? "hash_real" : "hash_int", i);
    }

    text_append(out, "    return hash;\n}\n\n");

    text_appendf(out, "static void %s_write(%s tuple)\n{\n", prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "%s    %s(tuple.field_%d);\n", i ? "    write_space();\n" : "",
                     signature[i] == 'r' ? "write_real" : "write_int", i);
    }

    text_append(out, "}\n\n");

    text_appendf(out, "static %s %s_read(int line)\n{\n    %s tuple;\n", type, prefix, type);

    for (int i = 0; i < count; i++)
    {
        text_appendf(out, "    tuple.field_%d = %s(line);\n", i,
                     signature[i] == 'r' ? "read_real" : "read_int");
    }

    text_append(out, "    return tuple;\n}\n");
}

/** Copia um arquivo do runtime embutido para a saída. */
void emit_text(TextBuffer *out, const unsigned char *text, unsigned int length)
{
    text_appendf(out, "%.*s", (int)length, (const char *)text);
}

/** Corpo do macro LOAD: como um elemento guardado vira valor da linguagem. */
const char *load_macro(const ElementKind *kind)
{
    if (!*kind->loader)
    {
        return "(stored)";
    }

    return !strcmp(kind->loader, "(int64_t)") ? "((int64_t)(stored))" : "decode_int32_inf(stored)";
}

/** Escreve o template de vetor para um tipo de elemento. */
void emit_array_template(TextBuffer *out, const ElementKind *kind)
{
    text_appendf(out, "\n#define ELEMENT %s\n#define VALUE %s\n", kind->c_type, kind->value_type);
    text_appendf(out, "#define ARRAY Array%s\n#define FN(name) array_%s##name\n", kind->pascal,
                 kind->suffix);
    text_appendf(out, "#define READ_ELEMENT %s\n#define WRITE_ELEMENT %s\n", kind->reader,
                 kind->writer);
    text_appendf(out, "#define LESS(left, right) %s(left, right)\n", kind->less_than);
    text_appendf(out, "#define LOAD(stored) %s\n#define KIND %d\n", load_macro(kind), kind->kind);
    emit_text(out, rt_arr_h, rt_arr_h_len);
    text_append(out, "#undef ELEMENT\n#undef VALUE\n#undef ARRAY\n#undef FN\n#undef READ_ELEMENT\n"
                     "#undef WRITE_ELEMENT\n#undef LESS\n#undef LOAD\n#undef KIND\n");
}

/** Escreve o template de uma estrutura (heap, fila, mapa ou grafo). */
void emit_template(TextBuffer *out, const TemplateUse *template_use)
{
    const ElementKind *element_kind = template_use->element;

    text_appendf(out, "\n#define FN(name) %s##name\n", template_use->prefix);

    if (template_use->kind == TEMPLATE_HEAP)
    {
        text_appendf(out, "#define ELEMENT %s\n#define HEAP %s\n", element_kind->c_type,
                     template_use->type_name);
        text_appendf(out, "#define LESS(left, right) %s(%s, %s)\n", element_kind->less_than,
                     template_use->descending ? "right" : "left",
                     template_use->descending ? "left" : "right");
        emit_text(out, rt_heap_h, rt_heap_h_len);
        text_append(out, "#undef ELEMENT\n#undef HEAP\n#undef LESS\n");
    }
    else if (template_use->kind == TEMPLATE_QUEUE)
    {
        text_appendf(out, "#define ELEMENT %s\n#define QUEUE %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_queue_h, rt_queue_h_len);
        text_append(out, "#undef ELEMENT\n#undef QUEUE\n");
    }
    else if (template_use->kind == TEMPLATE_MAP)
    {
        const char *hash_macro = element_kind->kind == 2
                                     ? format_string("%s_hash(key)", element_kind->suffix)
                                 : element_kind->kind == 1 ? "hash_real(key)"
                                                           : "hash_int(key)";
        const char *equal_macro = element_kind->kind == 2
                                      ? format_string("%s_equal(left, right)", element_kind->suffix)
                                      : "((left) == (right))";

        text_appendf(out, "#define KEY %s\n#define MAP_VALUE %s\n#define MAP %s\n",
                     element_kind->c_type, template_use->value->c_type, template_use->type_name);
        text_appendf(out, "#define HASH(key) %s\n#define KEYS_EQUAL(left, right) %s\n", hash_macro,
                     equal_macro);
        text_appendf(out, "#define KEY_ARRAY Array%s\n#define KEY_ARRAY_FN(name) array_%s##name\n",
                     element_kind->pascal, element_kind->suffix);
        emit_text(out, rt_map_h, rt_map_h_len);
        text_append(out,
                    "#undef KEY\n#undef MAP_VALUE\n#undef MAP\n#undef HASH\n#undef KEYS_EQUAL\n"
                    "#undef KEY_ARRAY\n#undef KEY_ARRAY_FN\n");
    }
    else if (template_use->kind == TEMPLATE_FENWICK)
    {
        text_appendf(out, "#define ELEMENT %s\n#define FENWICK %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_fenwick_h, rt_fenwick_h_len);
        text_append(out, "#undef ELEMENT\n#undef FENWICK\n");
    }
    else if (template_use->kind == TEMPLATE_SEGTREE || template_use->kind == TEMPLATE_RMQ)
    {
        static const char *const COMBINE_C[] = {"((left) < (right) ? (left) : (right))",
                                                "((left) > (right) ? (left) : (right))",
                                                "((left) + (right))"};
        int is_real = element_kind->kind == 1;
        const char *identity = template_use->descending == COMBINE_SUM ? "0"
                               : template_use->descending == COMBINE_MIN
                                   ? (is_real ? "INFINITY" : "INT64_MAX")
                                   : (is_real ? "-INFINITY" : "INT64_MIN");
        int is_rmq = template_use->kind == TEMPLATE_RMQ;

        text_appendf(out, "#define ELEMENT %s\n#define %s %s\n", element_kind->c_type,
                     is_rmq ? "RMQ" : "SEGTREE", template_use->type_name);
        text_appendf(out, "#define COMBINE(left, right) %s\n#define IDENTITY %s\n",
                     COMBINE_C[template_use->descending], identity);

        if (is_rmq)
        {
            emit_text(out, rt_rmq_h, rt_rmq_h_len);
        }
        else
        {
            emit_text(out, rt_segtree_h, rt_segtree_h_len);
        }

        text_appendf(out, "#undef ELEMENT\n#undef %s\n#undef COMBINE\n#undef IDENTITY\n",
                     is_rmq ? "RMQ" : "SEGTREE");
    }
    else
    {
        text_appendf(out, "#define WEIGHT %s\n#define GRAPH %s\n", element_kind->c_type,
                     template_use->type_name);
        emit_text(out, rt_graph_h, rt_graph_h_len);
        text_append(out, "#undef WEIGHT\n#undef GRAPH\n");
    }

    text_append(out, "#undef FN\n");
}

/** Gera o programa C completo: runtime, templates usados, funções e alg_main. */
void generate_c(Program *parsed_program, TextBuffer *out)
{
    TextBuffer prototypes = {0};
    TextBuffer function_bodies = {0};
    TextBuffer main_body = {0};

    generated_program = parsed_program;
    compute_writes(parsed_program);

    for (int i = 0; i < parsed_program->instances.count; i++)
    {
        mark_statements(&parsed_program->instances.items[i]->body);
    }

    /* primeiro gera o programa: só então se sabe quais templates de vetor ele usa */
    for (int i = 0; i < parsed_program->instances.count; i++)
    {
        FunctionInstance *instance = parsed_program->instances.items[i];
        gen_prototype(&prototypes, instance);
        text_append(&prototypes, ";\n");

        text_append(&function_bodies, "\n");
        gen_prototype(&function_bodies, instance);
        text_append(&function_bodies, "\n{\n");

        for (int j = 0; j < instance->locals.count; j++)
        {
            gen_variable_declaration(&function_bodies, instance->locals.items[j], 1, "");
        }

        gen_statements(&function_bodies, &instance->body, 1);
        text_append(&function_bodies, "}\n");
    }

    text_append(&main_body, "\nstatic void alg_main(void)\n{\n");

    /* variável que nenhuma função usa vira local do main: o gcc otimiza melhor */
    for (int i = 0; i < parsed_program->globals.count; i++)
    {
        if (!parsed_program->globals.items[i]->used_in_function)
        {
            gen_variable_declaration(&main_body, parsed_program->globals.items[i], 1, "");
        }
    }

    gen_statements(&main_body, &parsed_program->statements, 1);
    text_append(&main_body, "}\n");

    TextBuffer global_declarations = {0};

    for (int i = 0; i < parsed_program->globals.count; i++)
    {
        if (parsed_program->globals.items[i]->used_in_function)
        {
            gen_variable_declaration(&global_declarations, parsed_program->globals.items[i], 0,
                                     "static ");
        }
    }

    if (parsed_program->no_checks)
    {
        text_append(out, PRAGMAS);
    }

    text_appendf(out, "%.*s", (int)rt_core_h_len, (const char *)rt_core_h);

    for (int i = 0; i < tuples.count; i++)
    {
        emit_tuple(out, tuples.items[i]);
    }

    for (int kind_index = 0; kind_index < ELEMENT_KIND_COUNT; kind_index++)
    {
        if (element_used[kind_index])
        {
            emit_array_template(out, &ELEMENT_KINDS[kind_index]);
        }
    }

    for (int i = 0; i < tuples.count; i++)
    {
        emit_array_template(out, tuples.items[i]);
    }

    for (int i = 0; i < templates.count; i++)
    {
        emit_template(out, &templates.items[i]);
    }

    text_append(out, "\n/* ===== programa ===== */\n\n");

    TextBuffer *parts[] = {&global_declarations, &prototypes, &function_bodies, &main_body};

    for (int i = 0; i < 4; i++)
    {
        if (parts[i]->text)
        {
            text_append(out, parts[i]->text);
            text_append(out, i < 2 ? "\n" : "");
        }
    }
}
