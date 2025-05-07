#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct ASTNode {
  TokenType type;             // Type of the node
  char *value;                // String value of the node
  struct ASTNode *left;       // Left child
  struct ASTNode *right;      // Right child
  struct ASTNode *next;       // Next node in sequence
  struct ASTNode *prev;       // Previous node in sequence
  SourceLocation location;    // Source location information
} ASTNode;

// Create a new AST node
ASTNode *new_ast_node(TokenType type, const char *value, const SourceLocation *location);

// Parse the entire program
ASTNode *parse_all(Token **tokens);

// Connect previous pointers
void connect_prev_pointers(ASTNode *head);

// Free an AST node and all its children
void free_ast_node(ASTNode *node);

// Error handling and reporting
void parser_error(const Token *token, const char *format, ...);

#endif
