#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct ASTNode {
  TokenType type;        
  char *value;           
  struct ASTNode *left;  
  struct ASTNode *right; 
  struct ASTNode *next;  
  struct ASTNode *prev;  
} ASTNode;

ASTNode *new_ast_node(TokenType type, const char *value);

ASTNode *parse_all(Token **tokens);

void connect_prev_pointers(ASTNode *head);

void free_ast_node(ASTNode *node);

#endif 
