/*
  General structs used throughout the library
*/

/*
  List: a dynamic-length array
*/
typedef struct{
  void** items;
  int max;
  int n;
} List;

List* new_list(int max);
List* new_default_list();
void* get_from_list(List* ls,int i);
int add_to_list(List* ls,void* e);
void dealloc_list(List* ls);

/*
  Token: a symbol from the input code utilized by the parser
*/
typedef struct{
  char* text;
  int type;
  int line;
} Token;

void deallocate_token(Token* token);

// AST node types
typedef struct{
  void* data;
  int type;
} AstNode;

typedef struct{
  AstNode* node;
  List* list;
} AstListNode;

typedef struct{
  char text[256];
  char type[256];
} ValueNode;

typedef struct{
  char name[256];
  AstNode* type;
  List* body;
  List* args;
} FunctionNode;

typedef struct{
  List* keys;
  List* vals;
} TableNode;

typedef struct{
  AstNode* l;
  AstNode* r;
} AstAstNode;

typedef struct{
  AstNode* node;
  char* text;
} StringAstNode;

typedef struct{
  char name[256];
  AstNode* num1;
  AstNode* num2;
  AstNode* num3;
  List* body;
} FornumNode;

typedef struct{
  AstNode* lhs;
  AstNode* tuple;
  List* body;
} ForinNode;

typedef struct{
  char text[256];
  AstNode* l;
  AstNode* r;
} BinaryNode;
