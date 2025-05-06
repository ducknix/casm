#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *new_token(TokenType type, const char *value) {
  Token *token = (Token *)malloc(sizeof(Token));
  token->type = type;
  token->value = strdup(value);
  token->next = NULL;
  return token;
}

void free_tokens(Token *tokens) {
  Token *current = tokens;
  while (current != NULL) {
    Token *next = current->next;
    free(current->value);
    free(current);
    current = next;
  }
}

void add_token(Token **head, Token **tail, TokenType type, const char *value) {
  Token *token = new_token(type, value);
  if (*head == NULL)
    *head = token;
  else
    (*tail)->next = token;
  *tail = token;
}

Token *lexer(const char *source_code) {
  Token *head = NULL;
  Token *tail = NULL;
  size_t i = 0, len = strlen(source_code);

  while (i < len) {
    char current_char = source_code[i];

    if (isspace(current_char)) {
      i++;
      continue;
    }

    if (current_char == '\'') {
      while (i < len && source_code[i] != '\n')
        i++;
      continue;
    }

    if (current_char == '"') {
      size_t start = i;
      i++; 

      while (i < len && source_code[i] != '"') {
        if (source_code[i] == '\\' && i + 1 < len) {
          i += 2; 
        } else {
          i++;
        }
      }

      if (i < len && source_code[i] == '"') {
        i++; 

        char *string_literal = strndup(&source_code[start], i - start);
        add_token(&head, &tail, TOKEN_STRING, string_literal);
        free(string_literal);
        continue;
      } else {
        fprintf(stderr, "Unterminated string literal\n");

        i = start + 1;
        continue;
      }
    }

    if (current_char == '&') {
      if (strncmp(&source_code[i], "&strlen&", 8) == 0) {
        add_token(&head, &tail, TOKEN_STRLEN, "&strlen&");
        i += 8;
        continue;
      }
    }

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

    if (strncmp(&source_code[i], "syscall", 7) == 0 &&
        (i + 7 >= len || !isalnum(source_code[i + 7]))) {
      add_token(&head, &tail, TOKEN_SYS_CALL, "syscall");
      i += 7;
      continue;
    }

    if (strncmp(&source_code[i], "func", 4) == 0 &&
        (i + 4 >= len || isspace(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_LABEL_KW, "func");
      i += 4;
      continue;
    }


    if (strncmp(&source_code[i], "return", 6) == 0 &&
        (i + 6 >= len || !isalnum(source_code[i + 6]))) {
      add_token(&head, &tail, TOKEN_RETURN, "return");
      i += 6;
      continue;
    }

    if (strncmp(&source_code[i], "call", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_CALL, "call");
      i += 4;
      continue;
    }

    if (isalpha(current_char) || current_char == '&') {
      size_t start = i;
      while (i < len && (isalnum(source_code[i]) || source_code[i] == '_' || source_code[i] == '&'))
        i++;

      char *identifier = strndup(&source_code[start], i - start);

      if (identifier[0] == '&' && strlen(identifier) >= 2 &&
          isdigit(identifier[1])) {
        add_token(&head, &tail, TOKEN_IDENTIFIER, identifier);
      } else if (identifier[0] == 'r' && strlen(identifier) >= 2 &&
          isdigit(identifier[1])) {
        add_token(&head, &tail, TOKEN_IDENTIFIER, identifier);
      } else {
        add_token(&head, &tail, TOKEN_LABEL, identifier);
      }

      free(identifier);
      continue;
    }

    if (isdigit(current_char)) {
      size_t start = i;
      while (i < len && isdigit(source_code[i]))
        i++;
      char *number = strndup(&source_code[start], i - start);
      add_token(&head, &tail, TOKEN_NUMBER, number);
      free(number);
      continue;
    }

    add_token(&head, &tail, TOKEN_UNKNOWN, "UNKNOWN");
    i++;
  }

  return head;
}
