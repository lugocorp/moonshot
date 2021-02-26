
/*
  List of all possible tokens
*/

enum TOKENS{

  // Lua reserved keywords
  TK_AND, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,

  // Lua other terminal symbols
  TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
  TK_SHL, TK_SHR,
  TK_DBCOLON, TK_EOS,
  TK_FLT, TK_INT, TK_NAME, TK_STRING,

  // New tokens for vanilla lexing
  TK_PAREN, TK_CURLY, TK_SQUARE,
  TK_COMMENT, TK_QUOTE,
  TK_UNARY, TK_BINARY,
  TK_SPACE, TK_MISC,

  // New tokens specific to Moonshot
  TK_NEW, TK_FINAL, TK_TYPEDEF, TK_VAR,
  TK_INTERFACE, TK_CLASS, TK_EXTENDS, TK_IMPLEMENTS,
  TK_WHERE

};

/*
  List of all grammar rules
*/
enum RULES{
  // NULL,
  AST_BREAK, AST_TYPE_ANY,

  // char*
  AST_LABEL, AST_GOTO, AST_ID, AST_TYPE_BASIC,

  // List*
  AST_STMT, AST_DO, AST_LTUPLE, AST_TUPLE, AST_TYPE_TUPLE,

  // AstListNode*
  AST_REPEAT, AST_WHILE, AST_IF, AST_TYPE_FUNC,

  // ValueNode*
  AST_PRIMITIVE,

  // FunctionNode*
  AST_FUNCTION,

  // TableNode*
  AST_TABLE,

  // AstAstNode*
  AST_SET, AST_CALL, AST_SUB,

  // Binary node
  AST_BINARY, AST_DEFINE,

  // AstNode*
  AST_RETURN, AST_PAREN,

  // StringAstNode*
  AST_FIELD, AST_LOCAL, AST_UNARY, AST_TYPEDEF,

  // FornumNode*
  AST_FORNUM,

  // ForinNode*
  AST_FORIN,

  // Miscellaneous
  AST_UNKNOWN
};
