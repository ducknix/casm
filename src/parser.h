#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/**
 * @brief Represents a node in the Abstract Syntax Tree (AST).
 */
typedef struct ASTNode {
  TokenType type;        /**< Type of the token (from lexer) */
  char *value;           /**< Value of the node (literal, identifier, etc.) */
  struct ASTNode *left;  /**< Left child node */
  struct ASTNode *right; /**< Right child node */
  struct ASTNode *next;  /**< Next sibling node */
  struct ASTNode *prev;  /**< Previous sibling node (for backward traversal) */
} ASTNode;

/**
 * @brief Creates a new AST node with the given type and value.
 *
 * @param type Token type to assign
 * @param value String value of the node
 * @return Pointer to the newly allocated ASTNode
 */
ASTNode *new_ast_node(TokenType type, const char *value);

/**
 * @brief Parses the entire token stream and produces an abstract syntax tree.
 *
 * @param tokens Pointer to the token list
 * @return Pointer to the root ASTNode
 */
ASTNode *parse_all(Token **tokens);

/**
 * @brief Establishes backward traversal by linking prev pointers in the AST.
 *
 * @param head Pointer to the head of the AST list
 */
void connect_prev_pointers(ASTNode *head);

/**
 * @brief Recursively frees an AST node and all of its children.
 *
 * @param node Root of the AST subtree to free
 */
void free_ast_node(ASTNode *node);

#endif // PARSER_H
