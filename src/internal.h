#include <stdio.h>
#include <stdarg.h>
#define PRIMITIVE_STRING "string"
#define PRIMITIVE_FLOAT "float"
#define PRIMITIVE_BOOL "bool"
#define PRIMITIVE_INT "int"
#define PRIMITIVE_NIL "nil"

/*
  List: a dynamic-length array
*/
typedef struct
{
  void **items;
  int max;
  int n;
} List;

List *new_list(int max);
List *new_default_list();
void *get_from_list(List *ls, int i);
void *remove_from_list(List *ls, int i);
void append_all(List *ls, List *ls1);
void add_to_list(List *ls, void *e);
void dealloc_list(List *ls);

/*
  Map: a key-value object
*/
typedef struct
{
  char *k;
  void *v;
} Pair;
typedef struct
{
  Pair *data;
  int max;
  int n;
} Map;
Map *new_map(int max);
Map *new_default_map();
void *get_from_map(Map *m, char *k);
void *iterate_from_map(Map *m, int i);
void put_in_map(Map *m, char *k, void *v);
void dealloc_map(Map *m);

/*
  Token: a symbol from the input code utilized by the parser
*/
typedef struct
{
  char *text;
  int type;
  int line;
} Token;

void deallocate_token(Token *token);

// AST node types
typedef struct
{
  void *data;
  int type;
  int line;
} AstNode;

typedef struct
{
  AstNode *node;
  List *list;
} AstListNode;

typedef struct
{
  int is_constructor; // 1 if the function is a constructor
  AstNode *functype;  // Overall function type
  AstNode *name;      // An AST_LHS or AST_ID node representing the name, or NULL for constructors
  AstNode *type;      // Return type of the function (part of functype)
  List *body;         // List of AstNodes for the function body
  List *args;         // List of StringAstNodes for function parameters
} FunctionNode;

typedef struct
{
  List *keys; // List of strings
  List *vals; // List of AstNodes
} TableNode;

typedef struct
{
  AstNode *l;
  AstNode *r;
} AstAstNode;

typedef struct
{
  AstNode *node;
  char *text;
} StringAstNode;

typedef struct
{
  AstNode *num1;
  AstNode *num2;
  AstNode *num3;
  char *name;
  List *body;
} FornumNode;

typedef struct
{
  AstNode *lhs;
  AstNode *tuple;
  List *body;
} ForinNode;

typedef struct
{
  AstNode *l;
  AstNode *r;
  char *text;
} BinaryNode;

typedef struct
{
  AstNode *type; // Type representing the interface itself
  char *parent;  // Name of parent interface, or NULL if there is none
  char *name;    // Name of interface
  List *ls;      // List of AstNodes (AST_FUNCTION nodes)
} InterfaceNode;

typedef struct
{
  List *interfaces; // List of strings
  AstNode *type;    // Type representing the class itself
  char *parent;     // Name of parent class, or NULL if there is none
  char *name;       // Name of class
  List *ls;         // List of AstNodes
} ClassNode;

typedef struct
{
  AstNode *expr;
  AstNode *next;
  List *body;
} IfNode;

// Traversal algorithm structs
typedef struct
{
  AstNode *type; // Type that the registered type is equivalent to
  int relation;  // Type equivalence type (check enum Relations)
  char *name;    // Name of the registered type
  int scope;     // The index of the scope where this equivalence was defined
} EqualTypesNode;

typedef struct
{
  List *interfaces_registry; // List of InterfaceNodes
  List *functions_registry;  // List of FunctionNodes
  List *classes_registry;    // List of ClassNodes
  List *types_registry;      // List of strings
  List *defs;                // List of StringAstNodes representing local definitions
  void *data;                // Context node attached to this scope
  int type;                  // The type of this scope
} Scope;

// File requirements
typedef struct
{
  char *filename; // Filename of the required file
  AstNode *tree;  // Parsed AST tree
  List *tokens;   // Token list for file
  int completed;  // The highest step completed on this file
} Require;

// Enum for all traversal steps
enum STEPS
{
  STEP_TYPEDEF, // Step for processing type definitions, runs for each scope
  STEP_RELATE,  // Step for processing type relations, runs after typedef step for each scope
  STEP_CHECK,   // General validation step, runs after relate step for each scope
  STEP_OUTPUT   // Post-validation step for outputting Lua code, runs once after all validation
};

// Enum for all scope types
enum SCOPE_TYPES
{
  SCOPE_FUNCTION,
  SCOPE_CLASS,
  SCOPE_NONE
};

// Enum for all equivalent type relationships
enum RELATIONS
{
  RL_EXTENDS,
  RL_EQUALS,
  RL_IMPLEMENTS
};

// Enum for all possible tokens
enum TOKENS
{

  // Lua reserved keywords (starts with 0)
  TK_AND,
  TK_BREAK,
  TK_DO,
  TK_ELSE,
  TK_ELSEIF,
  TK_END,
  TK_FALSE,
  TK_FOR,
  TK_FUNCTION,
  TK_GOTO,
  TK_IF,
  TK_IN,
  TK_LOCAL,
  TK_NIL,
  TK_NOT,
  TK_OR,
  TK_REPEAT,
  TK_RETURN,
  TK_THEN,
  TK_TRUE,
  TK_UNTIL,
  TK_WHILE,

  // Lua other terminal symbols (starts with 22)
  TK_IDIV,
  TK_CONCAT,
  TK_DOTS,
  TK_EQ,
  TK_GE,
  TK_LE,
  TK_NE,
  TK_SHL,
  TK_SHR,
  TK_DBCOLON,
  TK_EOS,
  TK_FLT,
  TK_INT,
  TK_NAME,
  TK_STRING,

  // New tokens for vanilla lexing (starts with 37)
  TK_PAREN,
  TK_CURLY,
  TK_SQUARE,
  TK_QUOTE,
  TK_REQUIRE,
  TK_UNARY,
  TK_BINARY,
  TK_SPACE,
  TK_MISC,

  // New tokens specific to Moonshot
  TK_NEW,
  TK_FINAL,
  TK_TYPEDEF,
  TK_VAR,
  TK_INTERFACE,
  TK_CLASS,
  TK_EXTENDS,
  TK_IMPLEMENTS,
  TK_WHERE,
  TK_CONSTRUCTOR,
  TK_SUPER

};

// Enum for all grammar rules

enum RULES
{
  // Node's data is NULL
  AST_NONE,
  AST_BREAK,
  AST_TYPE_ANY,
  AST_TYPE_VARARG,

  // Node's data is char*
  AST_LABEL,
  AST_GOTO,
  AST_ID,
  AST_TYPE_BASIC,

  // Node's data is List*
  AST_STMT,
  AST_DO,
  AST_LTUPLE,
  AST_TYPE_TUPLE,
  AST_ELSE,

  // Node's data is AstListNode*
  AST_REPEAT,
  AST_WHILE,
  AST_TYPE_FUNC,
  AST_TUPLE,

  // Node's data is ClassNode*
  AST_CLASS,

  // Node's data is InterfaceNode*
  AST_INTERFACE,

  // Node's data is FunctionNode*
  AST_FUNCTION,

  // Node's data is TableNode*
  AST_TABLE,

  // Node's data is AstAstNode*
  AST_SET,
  AST_CALL,
  AST_SUB,

  // Node's data is Binary node
  AST_BINARY,
  AST_UNARY,
  AST_DEFINE,

  // Node's data is AstNode*
  AST_RETURN,
  AST_PAREN,
  AST_REQUIRE,
  AST_SUPER,
  AST_LIST,

  // Node's data is StringAstNode*
  AST_FIELD,
  AST_LOCAL,
  AST_TYPEDEF,
  AST_PRIMITIVE,

  // Node's data is FornumNode*
  AST_FORNUM,

  // Node's data is ForinNode*
  AST_FORIN,
  AST_ELSEIF,
  AST_IF,

  // Node's data is something else
  AST_UNKNOWN
};

// Implemented in moonshot.c
void add_error_internal(int line, const char *msg, va_list args);
char *format_string(int indent, const char *msg, va_list args);
void add_error(int line, const char *msg, ...);
int require_file(char *filename, int step);
char *collapse_string_list(List *ls);
void dealloc_token_buffer(List *ls);
char *strip_quotes(char *str);
char *copy_string(char *str);
char *string_from_int(int a);

// Implemented in tokenizer.c
void dealloc_token(Token *tk);
List *tokenize(FILE *f);

// Implemented in parser.c
AstNode *parse(List *ls);
AstNode *parse_function(AstNode *type, int include_body);
AstNode *parse_constructor(char *classname);
AstNode *parse_paren_or_tuple_function();
AstNode *parse_potential_tuple_lhs();
AstNode *parse_define(AstNode *type);
AstNode *parse_function_or_define();
AstNode *parse_call(AstNode *lhs);
AstNode *parse_table_or_list();
AstNode *parse_set_or_call();
AstNode *parse_interface();
AstNode *parse_typedef();
AstNode *parse_require();
AstNode *parse_repeat();
AstNode *parse_string();
AstNode *parse_number();
AstNode *parse_return();
AstNode *parse_fornum();
AstNode *parse_elseif();
AstNode *parse_super();
AstNode *parse_tuple();
AstNode *parse_while();
AstNode *parse_local();
AstNode *parse_table();
AstNode *parse_forin();
AstNode *parse_label();
AstNode *parse_break();
AstNode *parse_class();
AstNode *parse_list();
AstNode *parse_type();
AstNode *parse_stmt();
AstNode *parse_else();
AstNode *parse_list();
AstNode *parse_goto();
AstNode *parse_expr();
AstNode *parse_lhs();
AstNode *parse_if();
AstNode *parse_do();

// Implemented in nodes.c
FornumNode *new_fornum_node(char *name, AstNode *num1, AstNode *num2, AstNode *num3, List *body);
EqualTypesNode *new_equal_types_node(char *name, AstNode *type, int relation, int scope);
FunctionNode *new_function_node(AstNode *name, AstNode *type, List *args, List *body);
ClassNode *new_class_node(char *name, char *parent, List *interfaces, List *ls);
InterfaceNode *new_interface_node(char *name, char *parent, List *ls);
ForinNode *new_forin_node(AstNode *lhs, AstNode *tuple, List *body);
StringAstNode *new_primitive_node(char *text, const char *type);
BinaryNode *new_binary_node(char *text, AstNode *l, AstNode *r);
StringAstNode *new_string_ast_node(char *text, AstNode *ast);
IfNode *new_if_node(AstNode *expr, AstNode *next, List *body);
AstListNode *new_ast_list_node(AstNode *ast, List *list);
AstAstNode *new_ast_ast_node(AstNode *l, AstNode *r);
TableNode *new_table_node(List *keys, List *vals);
BinaryNode *new_unary_node(char *op, AstNode *e);
AstNode *new_node(int type, int line, void *data);
void dealloc_ast_type(AstNode *node);
void dealloc_ast_node(AstNode *node);

/*
 *   The parsing step should not have
 *   access to these functions or it may
 *   cause a segmentation fault.
 */
#ifndef MOONSHOT_PARSING

// Implemented in scopes.c
void push_function_scope(FunctionNode *node);
void register_interface(InterfaceNode *node);
InterfaceNode *interface_exists(char *name);
void register_function(FunctionNode *node);
StringAstNode *get_scoped_var(char *name);
FunctionNode *function_exists(char *name);
void register_primitive(const char *name);
int add_scoped_var(StringAstNode *node);
int field_defined_in_class(char *name);
void push_class_scope(ClassNode *node);
void register_class(ClassNode *node);
ClassNode *class_exists(char *name);
FunctionNode *get_function_scope();
FunctionNode *get_method_scope();
void register_type(char *name);
ClassNode *get_class_scope();
int type_exists(char *name);
void preempt_scopes();
void dealloc_scopes();
int get_num_scopes();
Scope *get_scope();
void init_scopes();
void push_scope();
void pop_scope();

// Implemented in types.c
int add_type_equivalence(char *name, AstNode *type, int relation);
int add_child_type(char *child, char *parent, int relation);
void quell_expired_scope_equivalences(int scope);
int is_primitive(AstNode *node, const char *type);
int types_equivalent(char *name, AstNode *type);
int compound_type_exists(AstNode *node);
List *get_equivalent_types(char *name);
int typed_match(AstNode *l, AstNode *r);
int is_variadic_function(List *args);
char *stringify_type(AstNode *node);
AstNode *get_type(AstNode *node);
char *base_type(char *name);
void print_types_graph();
void dealloc_types();
void init_types();

// Implemented in entities.c
FunctionNode *get_parent_method(ClassNode *clas, FunctionNode *method);
int methods_equivalent(FunctionNode *f1, FunctionNode *f2);
List *get_missing_class_methods(ClassNode *node);
FunctionNode *get_constructor(ClassNode *data);
Map *collapse_ancestor_class_fields(List *ls);
List *get_all_expected_fields(AstNode *node);
List *get_all_class_fields(ClassNode *data);
int num_constructors(ClassNode *data);

// Implemented in traversal.c
void traverse(AstNode *node, int step);
void dealloc_traverse();
void init_traverse();
int get_num_indents();
AstNode *any_type_const();
AstNode *int_type_const();
AstNode *bool_type_const();
AstNode *float_type_const();
void set_output(FILE *output);
void process_node_list(List *ls);
void process_list_primitive_node(AstNode *node);
void process_interface(AstNode *node);
void process_primitive(AstNode *node);
void process_function(AstNode *node);
void process_require(AstNode *node);
void process_typedef(AstNode *node);
void process_repeat(AstNode *node);
void process_ltuple(AstNode *node);
void process_return(AstNode *node);
void process_binary(AstNode *node);
void process_fornum(AstNode *node);
void process_elseif(AstNode *node);
void process_define(AstNode *node);
void process_class(AstNode *node);
void process_super(AstNode *node);
void process_break(AstNode *node);
void process_paren(AstNode *node);
void process_forin(AstNode *node);
void process_unary(AstNode *node);
void process_tuple(AstNode *node);
void process_table(AstNode *node);
void process_local(AstNode *node);
void process_while(AstNode *node);
void process_field(AstNode *node);
void process_label(AstNode *node);
void process_node(AstNode *node);
void process_goto(AstNode *node);
void process_call(AstNode *node);
void process_else(AstNode *node);
void process_set(AstNode *node);
void process_sub(AstNode *node);
void process_if(AstNode *node);
void process_do(AstNode *node);
void process_id(AstNode *node);

#endif
