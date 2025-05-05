#ifndef LEXER_H
#define LEXER_H

/**
 * @file lexer.h
 * @brief Header file for the lexer that tokenizes the source code.
 *
 * This file defines the token types and related structures necessary for
 * tokenizing source code. The lexer takes the source code as input and
 * produces a stream of tokens that can be analyzed and processed further.
 */

/**
 * @enum TokenType
 * @brief Enumerates the token types produced by the lexer.
 *
 * Token types represent different language constructs in the source code.
 * Each token type corresponds to a specific element in the language syntax.
 */
typedef enum {
  TOKEN_MOVE,       /**< MOVE command */
  TOKEN_ADD,        /**< ADD command */
  TOKEN_SUB,        /**< SUB command */
  TOKEN_COMPARE,    /**< COMPARE command */
  TOKEN_JUMP,       /**< JUMP command */
  TOKEN_JUMP_EQUAL, /**< JUMP_EQUAL command */
  TOKEN_RETURN,     /**< RETURN command */
  TOKEN_CALL,       /**< CALL command */
  TOKEN_SYS_CALL,   /**< SYS_CALL command */

  TOKEN_LABEL_KW,   /**< LABEL keyword (e.g., 'label' or 'func') */
  TOKEN_LABEL,      /**< Label name (e.g., function name) */
  TOKEN_IDENTIFIER, /**< Variable or function name */
  TOKEN_NUMBER,     /**< Numeric values */
  TOKEN_STRING,     /**< String literal (e.g., "hello") */
  TOKEN_STRLEN,     /**< &strlen& function */

  TOKEN_LPAREN,    /**< Left parenthesis '(' */
  TOKEN_RPAREN,    /**< Right parenthesis ')' */
  TOKEN_LBRACE,    /**< Left curly brace '{' */
  TOKEN_RBRACE,    /**< Right curly brace '}' */
  TOKEN_COMMA,     /**< Comma ',' */
  TOKEN_SEMICOLON, /**< Semicolon ';' */

  TOKEN_UNKNOWN /**< Unknown token (error case) */
} TokenType;

/**
 * @struct Token
 * @brief Structure representing a token produced by the lexer.
 *
 * Each token contains a type (TokenType), a value (string), and a pointer
 * to the next token in the list. Tokens are linked together to form a list
 * for further processing in the parser.
 */
typedef struct Token {
  TokenType type;     /**< Token type (e.g., TOKEN_NUMBER, TOKEN_IDENTIFIER) */
  char *value;        /**< Token value (e.g., "123", "myVar") */
  struct Token *next; /**< Pointer to the next token */
} Token;

/**
 * @brief Tokenizes the given source code into a list of tokens.
 *
 * This function processes the source code character by character and
 * generates a stream of tokens based on the language syntax. The lexer
 * identifies keywords, identifiers, literals, and other syntax elements
 * and returns them as a linked list of tokens.
 *
 * @param source_code The source code to be tokenized.
 * @return Token* A linked list of tokens representing the source code.
 */
Token *lexer(const char *source_code);

/**
 * @brief Frees the memory allocated for the token list.
 *
 * This function deallocates all the memory used by the linked list of tokens,
 * including the memory for each token's value string. It is important to call
 * this function to avoid memory leaks after tokenization.
 *
 * @param token The head of the token list to be freed.
 */
void free_tokens(Token *token);

#endif // LEXER_H
