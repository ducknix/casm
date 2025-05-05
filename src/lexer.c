#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Creates a new token of the specified type with the given value.
 *
 * This function allocates memory for a new token, initializes its type,
 * assigns its value, and sets the next pointer to NULL.
 *
 * @param type The type of the token (e.g., TOKEN_MOVE, TOKEN_ADD).
 * @param value The string value associated with the token.
 * @return Token* A pointer to the newly created token.
 */
Token *new_token(TokenType type, const char *value) {
  Token *token = (Token *)malloc(sizeof(Token));
  token->type = type;
  token->value = strdup(value);
  token->next = NULL;
  return token;
}

/**
 * @brief Frees the memory allocated for the list of tokens.
 *
 * This function traverses the linked list of tokens and frees the memory
 * for each token's value and the token structure itself.
 *
 * @param tokens The head of the token list to be freed.
 */
void free_tokens(Token *tokens) {
  Token *current = tokens;
  while (current != NULL) {
    Token *next = current->next;
    free(current->value);
    free(current);
    current = next;
  }
}

/**
 * @brief Adds a new token to the end of the linked list.
 *
 * This function creates a new token with the specified type and value,
 * and appends it to the linked list of tokens.
 *
 * @param head A pointer to the head of the token list.
 * @param tail A pointer to the tail of the token list.
 * @param type The type of the token (e.g., TOKEN_MOVE, TOKEN_ADD).
 * @param value The string value associated with the token.
 */
void add_token(Token **head, Token **tail, TokenType type, const char *value) {
  Token *token = new_token(type, value);
  if (*head == NULL)
    *head = token;
  else
    (*tail)->next = token;
  *tail = token;
}

/**
 * @brief Tokenizes the given source code into a linked list of tokens.
 *
 * This function processes the source code character by character, recognizing
 * various tokens such as keywords, identifiers, numbers, strings, and
 * operators. It creates a linked list of tokens that can be further analyzed by
 * the parser.
 *
 * @param source_code The source code to be tokenized.
 * @return Token* A linked list of tokens representing the source code.
 */
Token *lexer(const char *source_code) {
  Token *head = NULL;
  Token *tail = NULL;
  size_t i = 0, len = strlen(source_code);

  while (i < len) {
    char current_char = source_code[i];

    // Skip whitespace characters
    if (isspace(current_char)) {
      i++;
      continue;
    }

    // Skip single-line comments (denoted by a single quote)
    if (current_char == '\'') {
      while (i < len && source_code[i] != '\n')
        i++;
      continue;
    }

    // Handle string literals (enclosed in double quotes)
    if (current_char == '"') {
      size_t start = i;
      i++; // Skip opening double quote

      // Search for closing double quote
      while (i < len && source_code[i] != '"') {
        if (source_code[i] == '\\' && i + 1 < len) {
          i += 2; // Skip escape character and the following character
        } else {
          i++;
        }
      }

      if (i < len && source_code[i] == '"') {
        i++; // Include the closing quote

        // Extract the entire string literal
        char *string_literal = strndup(&source_code[start], i - start);
        add_token(&head, &tail, TOKEN_STRING, string_literal);
        free(string_literal);
        continue;
      } else {
        fprintf(stderr, "Unterminated string literal\n");
        // In case of error, continue with next character
        i = start + 1;
        continue;
      }
    }

    // Handle &strlen& token (replaces [counter])
    if (current_char == '&') {
      if (strncmp(&source_code[i], "&strlen&", 8) == 0) {
        add_token(&head, &tail, TOKEN_STRLEN, "&strlen&");
        i += 8;
        continue;
      }
    }

    // Backward compatibility for [counter]
    if (current_char == '[') {
      if (strncmp(&source_code[i], "[counter]", 9) == 0) {
        add_token(&head, &tail, TOKEN_STRLEN, "[counter]");
        i += 9;
        continue;
      }
    }

    // Handle various symbols
    if (current_char == '(') {
      add_token(&head, &tail, TOKEN_LPAREN, "(");
      i++;
      continue;
    }
    if (current_char == ')') {
      add_token(&head, &tail, TOKEN_RPAREN, ")");
      i++;
      continue;
    }
    if (current_char == '{') {
      add_token(&head, &tail, TOKEN_LBRACE, "{");
      i++;
      continue;
    }
    if (current_char == '}') {
      add_token(&head, &tail, TOKEN_RBRACE, "}");
      i++;
      continue;
    }
    if (current_char == ',') {
      add_token(&head, &tail, TOKEN_COMMA, ",");
      i++;
      continue;
    }
    if (current_char == ';') {
      add_token(&head, &tail, TOKEN_SEMICOLON, ";");
      i++;
      continue;
    }

    // Handle keywords like "move", "add", "sub", etc.
    if (strncmp(&source_code[i], "move", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_MOVE, "move");
      i += 4;
      continue;
    }
    if (strncmp(&source_code[i], "add", 3) == 0 &&
        (i + 3 >= len || !isalnum(source_code[i + 3]))) {
      add_token(&head, &tail, TOKEN_ADD, "add");
      i += 3;
      continue;
    }
    if (strncmp(&source_code[i], "sub", 3) == 0 &&
        (i + 3 >= len || !isalnum(source_code[i + 3]))) {
      add_token(&head, &tail, TOKEN_SUB, "sub");
      i += 3;
      continue;
    }
    if (strncmp(&source_code[i], "compare", 7) == 0 &&
        (i + 7 >= len || !isalnum(source_code[i + 7]))) {
      add_token(&head, &tail, TOKEN_COMPARE, "compare");
      i += 7;
      continue;
    }
    if (strncmp(&source_code[i], "jump_equal", 10) == 0 &&
        (i + 10 >= len || !isalnum(source_code[i + 10]))) {
      add_token(&head, &tail, TOKEN_JUMP_EQUAL, "jump_equal");
      i += 10;
      continue;
    }
    if (strncmp(&source_code[i], "jump", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_JUMP, "jump");
      i += 4;
      continue;
    }

    // Handle sys_call keyword
    if (strncmp(&source_code[i], "sys_call", 8) == 0 &&
        (i + 8 >= len || !isalnum(source_code[i + 8]))) {
      add_token(&head, &tail, TOKEN_SYS_CALL, "sys_call");
      i += 8;
      continue;
    }

    // Handle function (label) keywords
    if (strncmp(&source_code[i], "func", 4) == 0 &&
        (i + 4 >= len || isspace(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_LABEL_KW, "func");
      i += 4;
      continue;
    }

    // Keep old "label" keyword for backward compatibility
    if (strncmp(&source_code[i], "label", 5) == 0 &&
        (i + 5 >= len || isspace(source_code[i + 5]))) {
      add_token(&head, &tail, TOKEN_LABEL_KW, "label");
      i += 5;
      continue;
    }

    // Handle return keyword
    if (strncmp(&source_code[i], "return", 6) == 0 &&
        (i + 6 >= len || !isalnum(source_code[i + 6]))) {
      add_token(&head, &tail, TOKEN_RETURN, "return");
      i += 6;
      continue;
    }

    // Handle call keyword
    if (strncmp(&source_code[i], "call", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_CALL, "call");
      i += 4;
      continue;
    }

    // Handle identifiers
    if (isalpha(current_char)) {
      size_t start = i;
      while (i < len && (isalnum(source_code[i]) || source_code[i] == '_'))
        i++;

      char *identifier = strndup(&source_code[start], i - start);

      if (identifier[0] == 'r' && strlen(identifier) >= 2 &&
          isdigit(identifier[1])) {
        add_token(&head, &tail, TOKEN_IDENTIFIER, identifier);
      } else {
        add_token(&head, &tail, TOKEN_LABEL, identifier);
      }

      free(identifier);
      continue;
    }

    // Handle numbers
    if (isdigit(current_char)) {
      size_t start = i;
      while (i < len && isdigit(source_code[i]))
        i++;
      char *number = strndup(&source_code[start], i - start);
      add_token(&head, &tail, TOKEN_NUMBER, number);
      free(number);
      continue;
    }

    // If no matching token is found, mark it as unknown
    add_token(&head, &tail, TOKEN_UNKNOWN, "UNKNOWN");
    i++;
  }

  return head;
}
