#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// String representation of token types for error reporting
const char* token_type_strings[] = {
  "MOVE", "ADD", "SUB", "COMPARE", "JUMP", "JUMP_EQUAL", "JUMP_NOT_EQUAL",
  "RETURN", "CALL", "SYS_CALL", "FUNC", "LABEL", "IDENTIFIER", "NUMBER",
  "STRING", "STRLEN", "LPAREN", "RPAREN", "LBRACE", "RBRACE", "COMMA",
  "SEMICOLON", "UNKNOWN"
};

// Report error with location information
void report_error(const SourceLocation* loc, const char* format, ...) {
  va_list args;
  va_start(args, format);
  
  if (loc && loc->file) {
    fprintf(stderr, "%s:%d:%d: Error: ", loc->file, loc->line, loc->column);
  } else if (loc) {
    fprintf(stderr, "Line %d, Column %d: Error: ", loc->line, loc->column);
  } else {
    fprintf(stderr, "Error: ");
  }
  
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}

// Exit with error message
void error_exit(const char* message) {
  fprintf(stderr, "Fatal error: %s\n", message);
  fprintf(stderr, "Compilation aborted.\n");
  exit(EXIT_FAILURE);
}

Token *new_token(TokenType type, const char *value, int line, int column, const char* file) {
  Token *token = (Token *)malloc(sizeof(Token));
  if (!token) {
    error_exit("Memory allocation failed for token");
  }
  token->type = type;
  token->value = strdup(value);
  if (!token->value) {
    free(token);
    error_exit("Memory allocation failed for token value");
  }
  token->next = NULL;
  token->location.line = line;
  token->location.column = column;
  token->location.file = file;
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

void add_token(Token **head, Token **tail, TokenType type, const char *value, 
               int line, int column, const char* file) {
  Token *token = new_token(type, value, line, column, file);
  if (*head == NULL)
    *head = token;
  else
    (*tail)->next = token;
  *tail = token;
}

Token *lexer(const char *source_code, const char* filename) {
  Token *head = NULL;
  Token *tail = NULL;
  size_t i = 0, len = strlen(source_code);
  int line = 1;  // Start from line 1
  int column = 1;

  while (i < len) {
    char current_char = source_code[i];
    int start_column = column;  // Save the starting column for this token

    // Handle newlines for line counting
    if (current_char == '\n') {
      line++;
      column = 1;
      i++;
      continue;
    }

    if (isspace(current_char)) {
      column++;
      i++;
      continue;
    }
    
    if (current_char == '/' && i + 1 < len) {
      // Single-line comment: //
      if (source_code[i + 1] == '/') {
        i += 2;
        column += 2;
        while (i < len && source_code[i] != '\n') {
          i++;
          column++;
        }
        continue;
      }
      // Multi-line comment: /* */
      else if (source_code[i + 1] == '*') {
        int comment_start_line = line;
        int comment_start_column = column;
        i += 2;
        column += 2;
        while (i + 1 < len && !(source_code[i] == '*' && source_code[i + 1] == '/')) {
          if (source_code[i] == '\n') {
            line++;
            column = 1;
          } else {
            column++;
          }
          i++;
        }
        
        if (i + 1 < len) {
          i += 2;  // Skip past the */
          column += 2;
        } else {
          SourceLocation loc = {comment_start_line, comment_start_column, filename};
          report_error(&loc, "Unclosed multi-line comment");
          error_exit("Syntax error");
        }
        
        continue;
      }
    }

    if (current_char == '"') {
      size_t start = i;
      int string_start_line = line;
      int string_start_column = column;
      i++;
      column++;

      while (i < len && source_code[i] != '"') {
        if (source_code[i] == '\n') {
          line++;
          column = 1;
          i++;
        } else if (source_code[i] == '\\' && i + 1 < len) {
          i += 2;
          column += 2;
        } else {
          i++;
          column++;
        }
      }

      if (i < len && source_code[i] == '"') {
        i++;  // Move past the closing quote
        column++;

        char *string_literal = strndup(&source_code[start], i - start);
        add_token(&head, &tail, TOKEN_STRING, string_literal, string_start_line, string_start_column, filename);
        free(string_literal);
        continue;
      } else {
        SourceLocation loc = {string_start_line, string_start_column, filename};
        report_error(&loc, "Unterminated string literal");
        error_exit("Syntax error");
      }
    }

    if (current_char == '&') {
      if (strncmp(&source_code[i], "&strlen&", 8) == 0) {
        add_token(&head, &tail, TOKEN_STRLEN, "&strlen&", line, column, filename);
        i += 8;
        column += 8;
        continue;
      }
    }

    if (current_char == '(') {
      add_token(&head, &tail, TOKEN_LPAREN, "(", line, column, filename);
      i++;
      column++;
      continue;
    }
    if (current_char == ')') {
      add_token(&head, &tail, TOKEN_RPAREN, ")", line, column, filename);
      i++;
      column++;
      continue;
    }
    if (current_char == '{') {
      add_token(&head, &tail, TOKEN_LBRACE, "{", line, column, filename);
      i++;
      column++;
      continue;
    }
    if (current_char == '}') {
      add_token(&head, &tail, TOKEN_RBRACE, "}", line, column, filename);
      i++;
      column++;
      continue;
    }
    if (current_char == ',') {
      add_token(&head, &tail, TOKEN_COMMA, ",", line, column, filename);
      i++;
      column++;
      continue;
    }
    if (current_char == ';') {
      add_token(&head, &tail, TOKEN_SEMICOLON, ";", line, column, filename);
      i++;
      column++;
      continue;
    }

    // Keywords
    if (strncmp(&source_code[i], "move", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_MOVE, "move", line, column, filename);
      i += 4;
      column += 4;
      continue;
    }
    if (strncmp(&source_code[i], "add", 3) == 0 &&
        (i + 3 >= len || !isalnum(source_code[i + 3]))) {
      add_token(&head, &tail, TOKEN_ADD, "add", line, column, filename);
      i += 3;
      column += 3;
      continue;
    }
    if (strncmp(&source_code[i], "sub", 3) == 0 &&
        (i + 3 >= len || !isalnum(source_code[i + 3]))) {
      add_token(&head, &tail, TOKEN_SUB, "sub", line, column, filename);
      i += 3;
      column += 3;
      continue;
    }
    if (strncmp(&source_code[i], "compare", 7) == 0 &&
        (i + 7 >= len || !isalnum(source_code[i + 7]))) {
      add_token(&head, &tail, TOKEN_COMPARE, "compare", line, column, filename);
      i += 7;
      column += 7;
      continue;
    }
    if (strncmp(&source_code[i], "jump_not_equal", 14) == 0 &&
        (i + 14 >= len || !isalnum(source_code[i + 14]))) {
      add_token(&head, &tail, TOKEN_JUMP_NOT_EQUAL, "jump_not_equal", line, column, filename);
      i += 14;
      column += 14;
      continue;
    }
    if (strncmp(&source_code[i], "jump_equal", 10) == 0 &&
        (i + 10 >= len || !isalnum(source_code[i + 10]))) {
      add_token(&head, &tail, TOKEN_JUMP_EQUAL, "jump_equal", line, column, filename);
      i += 10;
      column += 10;
      continue;
    }
    if (strncmp(&source_code[i], "jump", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_JUMP, "jump", line, column, filename);
      i += 4;
      column += 4;
      continue;
    }
    if (strncmp(&source_code[i], "syscall", 7) == 0 &&
        (i + 7 >= len || !isalnum(source_code[i + 7]))) {
      add_token(&head, &tail, TOKEN_SYS_CALL, "syscall", line, column, filename);
      i += 7;
      column += 7;
      continue;
    }
    if (strncmp(&source_code[i], "func", 4) == 0 &&
        (i + 4 >= len || isspace(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_LABEL_KW, "func", line, column, filename);
      i += 4;
      column += 4;
      continue;
    }
    if (strncmp(&source_code[i], "return", 6) == 0 &&
        (i + 6 >= len || !isalnum(source_code[i + 6]))) {
      add_token(&head, &tail, TOKEN_RETURN, "return", line, column, filename);
      i += 6;
      column += 6;
      continue;
    }
    if (strncmp(&source_code[i], "call", 4) == 0 &&
        (i + 4 >= len || !isalnum(source_code[i + 4]))) {
      add_token(&head, &tail, TOKEN_CALL, "call", line, column, filename);
      i += 4;
      column += 4;
      continue;
    }

    // Identifiers and labels
    if (isalpha(current_char) || current_char == '&') {
      size_t start = i;
      int id_start_column = column;
      while (i < len && (isalnum(source_code[i]) || source_code[i] == '_' || source_code[i] == '&')) {
        i++;
        column++;
      }

      char *identifier = strndup(&source_code[start], i - start);

      if (identifier[0] == '&' && strlen(identifier) >= 2 &&
          isdigit(identifier[1])) {
        add_token(&head, &tail, TOKEN_IDENTIFIER, identifier, line, id_start_column, filename);
      } else if (identifier[0] == 'r' && strlen(identifier) >= 2 &&
          isdigit(identifier[1])) {
        add_token(&head, &tail, TOKEN_IDENTIFIER, identifier, line, id_start_column, filename);
      } else {
        add_token(&head, &tail, TOKEN_LABEL, identifier, line, id_start_column, filename);
      }

      free(identifier);
      continue;
    }

    // Numbers
    if (isdigit(current_char)) {
      size_t start = i;
      int num_start_column = column;
      while (i < len && isdigit(source_code[i])) {
        i++;
        column++;
      }
      char *number = strndup(&source_code[start], i - start);
      add_token(&head, &tail, TOKEN_NUMBER, number, line, num_start_column, filename);
      free(number);
      continue;
    }

    // Unknown character
    SourceLocation loc = {line, column, filename};
    report_error(&loc, "Unexpected character: '%c'", current_char);
    add_token(&head, &tail, TOKEN_UNKNOWN, "UNKNOWN", line, column, filename);
    i++;
    column++;
  }

  return head;
}
