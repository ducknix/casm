#ifndef LEXER_H
#define LEXER_H

typedef enum {
  TOKEN_MOVE,       
  TOKEN_ADD,        
  TOKEN_SUB,        
  TOKEN_COMPARE,    
  TOKEN_JUMP,       
  TOKEN_JUMP_EQUAL, 
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

typedef struct Token {
  TokenType type;     
  char *value;        
  struct Token *next; 
} Token;

Token *lexer(const char *source_code);

void free_tokens(Token *token);

#endif 
