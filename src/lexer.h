#ifndef LEXER_H
#define LEXER_H

typedef enum {
  TOKEN_MOVE,       
  TOKEN_ADD,        
  TOKEN_SUB,        
  TOKEN_COMPARE,    
  TOKEN_JUMP,       
  TOKEN_JUMP_EQUAL,
  TOKEN_JUMP_NOT_EQUAL, 
  TOKEN_RETURN,     
  TOKEN_CALL,       
  TOKEN_SYS_CALL,   

  TOKEN_LABEL_KW,   
  TOKEN_LABEL,      
  TOKEN_IDENTIFIER, 
  TOKEN_NUMBER,     
  TOKEN_STRING,     
  TOKEN_STRLEN,     

  TOKEN_LPAREN,    
  TOKEN_RPAREN,    
  TOKEN_LBRACE,    
  TOKEN_RBRACE,    
  TOKEN_COMMA,     
  TOKEN_SEMICOLON, 

  TOKEN_UNKNOWN 
} TokenType;

typedef struct SourceLocation {
  int line;           
  int column;         
  const char* file;   
} SourceLocation;

typedef struct Token {
  TokenType type;     
  char *value;        
  struct Token *next; 
  SourceLocation location;
} Token;

// Error handling
void report_error(const SourceLocation* loc, const char* format, ...);
void error_exit(const char* message);

// Token creation and management
Token *new_token(TokenType type, const char *value, int line, int column, const char* file);
void free_tokens(Token *token);
Token *lexer(const char *source_code, const char* filename);

// String constants for token types (for debugging)
extern const char* token_type_strings[];

#endif
