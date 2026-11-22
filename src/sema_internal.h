/* Checagem semântica: tipos, estado e funções compartilhados entre os arquivos do módulo. */
#ifndef SEMA_INTERNAL_H
#define SEMA_INTERNAL_H

#include "alg.h"

typedef VEC(Function *) FunctionList;

typedef struct Scope
{
    VEC(Variable *) variables; /* em ordem de declaração */
    Variable **slots;          /* tabela hash nome → variável (endereçamento aberto) */
    int capacity;              /* potência de 2; no máximo metade ocupada */
    struct Scope *parent;
} Scope;

#define FAIL_AT_NODE(e, ...) fail_at((e)->line, (e)->column, __VA_ARGS__)

extern Program *checked_program;
extern FunctionList functions;
extern int loop_depth;
extern Scope global_scope;
extern Scope *function_scope;
extern Scope *current_scope;
extern FunctionInstance *current_instance;
extern VariableList pending_dimensions;
extern Type INT_TYPE;
extern Type REAL_TYPE;
extern Type STRING_TYPE;
extern Type VOID_TYPE;
extern Type CHAR_TYPE;

uint64_t name_hash(const char *name);
Variable *scope_find(Scope *scope, const char *name);
void scope_insert(Scope *scope, Variable *variable);
Type *new_type(TypeKind kind);
Type *array_type(Type *elem, int nd);
Type *cell_root(Type *type);
Type *resolve_unknown(Type *type);
int is_number(Type *type);
int is_string(Type *type);
Type *new_string_type(void);
Type *value_of_cell(Type *cell);
const char *type_name(Type *type);
int same_tuple(Type *left_type, Type *right_type);
void fill_cell(Type *cell, Type *type);
Type *owned_type(Type *type);
void unify_elements(Type *target, Type *source, Expression *location);
void unify_types(Type *target, Type *source, Expression *location);
void store_in_cell(Type *cell, Type *type, Expression *location);
Type *new_structure_cell(void);
Variable *lookup_variable(const char *name);
Variable *lookup_loop_variable(const char *name);
Variable *declare_in_scope(Scope *scope, const char *name, Type *ty, int line);
Variable *declare_variable(const char *name, Type *ty, int line);
Function *find_function(const char *name);
void clone_expression_list(ExpressionList *destination, ExpressionList *source);
Expression *clone_expression(Expression *expr);
void clone_statement_list(StatementList *destination, StatementList *source);
Statement *clone_statement(Statement *stmt);
Type *check_int(Expression *expr);
Type *check_number(Expression *expr);
Type *check_array(Expression *expr);
Type *check_generator(Expression *generator);
Type *check_reduce(Expression *expr, Builtin builtin);
int cells_compatible(Type *left_type, Type *right_type);
int same_signature(Type *left_type, Type *right_type);
void check_pending_dimensions(int from);
FunctionInstance *find_instance(Function *function, Type **argument_types, int count);
FunctionInstance *new_instance(Function *function, Type **argument_types);
Type *check_user_call(Expression *expr, Function *function);
void check_argument_count(Expression *expr, int lo, int hi);
int is_word(Expression *argument, Builtin builtin);
Type *check_binary(Expression *expr);
Type *taken_value(Type *cell);
Type *check_range_structure(Expression *expr, Builtin builtin);
Type *check_method(Expression *expr);
Type *check_expression(Expression *expr);
void check_read_argument(Expression *argument);
int holds_strings(Expression *target);
void check_assignment(Statement *stmt);
Variable *loop_variable(const char *name, Type *type, Statement *stmt);
void check_for_in(Statement *stmt);
void check_statement(Statement *stmt, int top, int *seen_code);
void check_statements(StatementList *statements, int top);
void check_program(Program *parsed_program);

#endif
