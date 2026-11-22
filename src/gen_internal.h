/* Geração de C: tipos, estado e funções compartilhados entre os arquivos do módulo. */
#ifndef GEN_INTERNAL_H
#define GEN_INTERNAL_H

#include "alg.h"

/* ---------- tipo de elemento (vetor, heap, fila, mapa, grafo) ---------- */

typedef struct
{
    const char *suffix;     /* prefixo das funções: array_<suffix>_push */
    const char *pascal;     /* nome do tipo: Array<Pascal> */
    const char *c_type;     /* tipo guardado na memória */
    const char *value_type; /* tipo do valor na linguagem (contas, o que sai do vetor) */
    const char *reader;     /* leitura de um elemento */
    const char *writer;     /* escrita de um elemento */
    const char *less_than;  /* ordem: LESS_THAN ou tuple_<campos>_less */
    const char *loader;     /* guardado → valor: "" ou conversão */
    const char *signature;  /* tupla: tipos dos campos ("ir" = int, real) */
    int kind;               /* 0 inteiro, 1 real, 2 tupla */
} ElementKind;

typedef VEC(ElementKind *) ElementKindList;

enum
{
    ELEMENT_U8,
    ELEMENT_I8,
    ELEMENT_U16,
    ELEMENT_I16,
    ELEMENT_U32,
    ELEMENT_I32,
    ELEMENT_I32INF,
    ELEMENT_I64,
    ELEMENT_REAL,
    ELEMENT_KIND_COUNT
};

/* ---------- estruturas: cada combinação de tipos instancia um template ---------- */

enum
{
    TEMPLATE_HEAP,
    TEMPLATE_QUEUE,
    TEMPLATE_MAP,
    TEMPLATE_GRAPH,
    TEMPLATE_FENWICK,
    TEMPLATE_SEGTREE,
    TEMPLATE_RMQ
};

typedef struct
{
    int kind;
    char prefix[96];            /* prefixo das funções: heap_i64_min_push */
    char type_name[96];         /* nome do tipo: HeapI64Min */
    const ElementKind *element; /* elemento / chave / peso */
    const ElementKind *value;   /* valor do mapa */
    int descending;             /* heap(maior) */
} TemplateUse;

typedef VEC(TemplateUse) TemplateUseList;

/* ---------- variáveis ---------- */

typedef VariableList VariableSet;

extern int temp_counter;
extern const ElementKind ELEMENT_KINDS[ELEMENT_KIND_COUNT];
extern Program *generated_program;
extern int element_used[ELEMENT_KIND_COUNT];
extern ElementKindList tuples;
extern TemplateUseList templates;
extern int strings_in_containers;
extern Expression *cached_map_index;
extern int cached_map_pointer;
extern const ElementKind STRING_ELEMENT;
extern VariableSet ensured_lazy;
extern const char *PRAGMAS;

void indent(TextBuffer *out, int depth);
void emit_identifier(TextBuffer *out, const char *prefix, const char *name);
Type *resolve_unknown_type(Type *type);
char *format_string(const char *format_text, const char *argument);
const ElementKind *tuple_element_kind(Type *type);
int is_text_type(Type *type);
int scalar_element_kind(Type *cell);
const ElementKind *cell_element_kind(Type *cell);
const char *array_suffix(Type *array_type);
const char *array_pascal(Type *array_type);
int is_checked_int32(Type *cell);
const TemplateUse *use_template(int kind, const ElementKind *element_kind,
                                const ElementKind *value_kind, int descending);
const TemplateUse *structure_template(Type *type);
const char *template_name(Type *type);
const char *template_type(Type *type);
int is_structure_type(Type *type);
void emit_c_type(TextBuffer *out, Type *type);
void emit_c_string(TextBuffer *out, const char *text);
int set_contains(VariableSet *set, Variable *variable);
int set_add(VariableSet *set, Variable *variable);
void emit_lazy_allocation(TextBuffer *out, Variable *variable, int line);
void emit_lazy_ensure(TextBuffer *out, Variable *variable, int line);
void gen_variable(TextBuffer *out, Variable *variable, int line);
void gen_array_pointer(TextBuffer *out, Expression *expr);
const char *c_operator(TokenKind operation);
void gen_call2(TextBuffer *out, const char *function_suffix, Expression *item, Expression *right);
void gen_tuple_comparison(TextBuffer *out, Expression *expr);
void gen_binary(TextBuffer *out, Expression *expr);
void gen_generator_open(TextBuffer *out, Expression *generator, int temp_id);
void gen_reduce(TextBuffer *out, Expression *expr, Builtin builtin);
void gen_loaded(TextBuffer *out, Type *array, const char *element);
void gen_prefix(TextBuffer *out, Expression *expr);
void gen_search(TextBuffer *out, Expression *expr);
void gen_range_structure(TextBuffer *out, Expression *expr);
void gen_call(TextBuffer *out, Expression *expr);
void gen_lvalue(TextBuffer *out, Expression *expr);
void gen_stored_value(TextBuffer *out, Type *cell, Expression *value);
void gen_method_call(TextBuffer *out, Expression *expr);
void gen_real(TextBuffer *out, double number);
void gen_expression(TextBuffer *out, Expression *expr);
void gen_read(TextBuffer *out, Expression *argument, int depth);
void gen_write(TextBuffer *out, Expression *expr, int depth);
void gen_block(TextBuffer *out, StatementList *statements, int depth);
void walk_expression(Expression *expr, VariableSet *lazy_arrays, VariableSet *writes);
void walk_statement(Statement *stmt, VariableSet *lazy_arrays, VariableSet *writes);
void walk_statements(StatementList *statements, VariableSet *lazy_arrays, VariableSet *writes);
void compute_writes(Program *parsed_program);
int dimensions_change_in(Variable *variable, VariableSet *writes);
void gen_loop_hoisted(TextBuffer *out, Statement *stmt, int depth);
void gen_assign_line(TextBuffer *out, Variable *variable, int line, int depth,
                     const char *value_format, ...);
void gen_for_in(TextBuffer *out, Statement *stmt, int depth);
void gen_tuple_assignment(TextBuffer *out, Statement *stmt, int depth);
void gen_loop(TextBuffer *out, Statement *stmt, int depth);
void gen_statement(TextBuffer *out, Statement *stmt, int depth);
void gen_statements(TextBuffer *out, StatementList *statements, int depth);
void gen_prototype(TextBuffer *out, FunctionInstance *instance);
void gen_variable_declaration(TextBuffer *out, Variable *variable, int depth, const char *prefix);
void emit_tuple(TextBuffer *out, const ElementKind *kind);
void emit_text(TextBuffer *out, const unsigned char *text, unsigned int length);
const char *load_macro(const ElementKind *kind);
void emit_array_template(TextBuffer *out, const ElementKind *kind);
void emit_template(TextBuffer *out, const TemplateUse *template_use);
void mark_expression(Expression *expr);
void mark_statements(StatementList *statements);
void generate_c(Program *parsed_program, TextBuffer *out);

#endif
