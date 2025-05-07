#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Implement the parser_error function declared in parser.h
void parser_error(const Token *token, const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  if (token && token->location.file) {
    fprintf(stderr, "%s:%d:%d: Parser error: ", 
            token->location.file, token->location.line, token->location.column);
  } else if (token) {
    fprintf(stderr, "Line %d, Column %d: Parser error: ", 
            token->location.line, token->location.column);
  } else {
    fprintf(stderr, "Parser error: ");
  }
  
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}

// Updated to include location information as declared in parser.h
ASTNode *new_ast_node(TokenType type, const char *value, const SourceLocation *location) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
  if (!node) {
    error_exit("Memory allocation failed for AST node");
  }
  
  node->type = type;
  node->value = strdup(value);
  if (!node->value) {
    free(node);
    error_exit("Memory allocation failed for AST node value");
  }
  
  node->left = NULL;
  node->right = NULL;
  node->next = NULL;
  node->prev = NULL;
  
  // Copy location information if provided
  if (location) {
    node->location = *location;
  } else {
    // Initialize with default values if no location provided
    node->location.line = 0;
    node->location.column = 0;
    node->location.file = NULL;
  }
  
  return node;
}

void free_ast_node(ASTNode *node) {
  if (node) {
    free(node->value);
    free_ast_node(node->left);
    free_ast_node(node->right);
    free_ast_node(node->next);
    free(node);
  }
}

void connect_prev_pointers(ASTNode *head) {
  if (!head)
    return;

  ASTNode *current = head;
  ASTNode *prev = NULL;

  while (current) {
    current->prev = prev;

    if (current->type == TOKEN_LABEL && current->left &&
        current->left->type == TOKEN_LBRACE) {
      ASTNode *block_content = current->left->left;
      ASTNode *block_prev = NULL;

      while (block_content) {
        block_content->prev = block_prev;
        block_prev = block_content;
        block_content = block_content->next;
      }
    }

    prev = current;
    current = current->next;
  }
}

ASTNode *parse_expression(Token **tokens) {
  if (!tokens || !*tokens)
    return NULL;
  
  Token *current = *tokens;

  if (current->type == TOKEN_NUMBER || current->type == TOKEN_IDENTIFIER ||
      current->type == TOKEN_LABEL || current->type == TOKEN_STRING ||
      current->type == TOKEN_STRLEN) {
    *tokens = current->next;
    return new_ast_node(current->type, current->value, &current->location);
  }

  if (current->type == TOKEN_LPAREN) {
    Token *lparen_token = current;
    *tokens = current->next;
    ASTNode *expr = parse_expression(tokens);
    
    if (!expr) {
      parser_error(lparen_token, "Expected expression after '('");
      return NULL;
    }
    
    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : lparen_token, 
                  "Expected ')' to match opening parenthesis at line %d, column %d", 
                  lparen_token->location.line, lparen_token->location.column);
      free_ast_node(expr);
      return NULL;
    }
    return expr;
  }

  if (current->type == TOKEN_COMMA) {
    *tokens = current->next;
    return parse_expression(tokens);
  }

  parser_error(current, "Unexpected expression token: %s", current->value);
  return NULL;
}

ASTNode *parse_syscall_params(Token **tokens) {
  if (!*tokens) {
    parser_error(NULL, "Unexpected end of input while parsing syscall parameters");
    return NULL;
  }
  
  if ((*tokens)->type != TOKEN_LPAREN) {
    parser_error(*tokens, "Expected '(' after syscall, found '%s'", (*tokens)->value);
    return NULL;
  }

  Token *lparen_token = *tokens;
  *tokens = (*tokens)->next;

  ASTNode *params_head = NULL;
  ASTNode *params_tail = NULL;

  if (*tokens && (*tokens)->type != TOKEN_RPAREN) {
    ASTNode *param = parse_expression(tokens);
    if (!param) {
      parser_error(*tokens ? *tokens : lparen_token, "Expected parameter in syscall");
      return NULL;
    }

    params_head = params_tail = param;

    while (*tokens && (*tokens)->type == TOKEN_COMMA) {
      Token *comma_token = *tokens;
      *tokens = (*tokens)->next;

      param = parse_expression(tokens);
      if (!param) {
        parser_error(*tokens ? *tokens : comma_token, "Expected parameter after comma in syscall");
        free_ast_node(params_head);
        return NULL;
      }

      params_tail->next = param;
      param->prev = params_tail;
      params_tail = param;
    }
  }

  if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
    *tokens = (*tokens)->next;
  } else {
    parser_error(*tokens ? *tokens : lparen_token, 
                "Expected ')' after syscall parameters to match opening parenthesis at line %d, column %d",
                lparen_token->location.line, lparen_token->location.column);
    free_ast_node(params_head);
    return NULL;
  }

  return params_head;
}

ASTNode *parse_statement(Token **tokens) {
  if (!tokens || !*tokens)
    return NULL;
  
  Token *current = *tokens;

  if (current->type == TOKEN_MOVE || current->type == TOKEN_ADD ||
      current->type == TOKEN_SUB || current->type == TOKEN_COMPARE) {

    TokenType op_type = current->type;
    Token *op_token = current;
    *tokens = current->next;

    ASTNode *node = new_ast_node(op_type, current->value, &current->location);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : op_token, 
                  "Expected '(' after '%s'", op_token->value);
      free_ast_node(node);
      return NULL;
    }

    node->left = parse_expression(tokens);
    if (!node->left) {
      parser_error(*tokens ? *tokens : op_token,
                  "Expected first operand for '%s'", op_token->value);
      free_ast_node(node);
      return NULL;
    }

    if (*tokens && (*tokens)->type == TOKEN_COMMA) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : op_token, 
                  "Expected ',' between operands for '%s'", op_token->value);
      free_ast_node(node);
      return NULL;
    }

    node->right = parse_expression(tokens);
    if (!node->right) {
      parser_error(*tokens ? *tokens : op_token,
                  "Expected second operand for '%s'", op_token->value);
      free_ast_node(node);
      return NULL;
    }

    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : op_token, 
                  "Expected ')' to close expression for '%s'", op_token->value);
      free_ast_node(node);
      return NULL;
    }

    return node;
  }

  if (current->type == TOKEN_JUMP || current->type == TOKEN_JUMP_EQUAL || 
      current->type == TOKEN_JUMP_NOT_EQUAL) {
    TokenType op_type = current->type;
    Token *jump_token = current;
    *tokens = current->next;

    ASTNode *node = new_ast_node(op_type, current->value, &current->location);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : jump_token, 
                  "Expected '(' after '%s'", jump_token->value);
      free_ast_node(node);
      return NULL;
    }

    node->left = parse_expression(tokens);
    if (!node->left) {
      parser_error(*tokens ? *tokens : jump_token,
                  "Expected jump target for '%s'", jump_token->value);
      free_ast_node(node);
      return NULL;
    }

    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : jump_token, 
                  "Expected ')' to close jump target for '%s'", jump_token->value);
      free_ast_node(node);
      return NULL;
    }

    return node;
  }

  if (current->type == TOKEN_RETURN || strcmp(current->value, "return") == 0) {
    Token *return_token = current;
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_RETURN, "return", &current->location);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      Token *lparen_token = *tokens;
      *tokens = (*tokens)->next;

      if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
        *tokens = (*tokens)->next;
      } else {
        parser_error(*tokens ? *tokens : lparen_token, 
                    "Expected ')' after 'return()' to match opening parenthesis at line %d, column %d",
                    lparen_token->location.line, lparen_token->location.column);
        free_ast_node(node);
        return NULL;
      }
    }

    return node;
  }

  if (current->type == TOKEN_SYS_CALL || strcmp(current->value, "syscall") == 0) {
    Token *syscall_token = current;
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_SYS_CALL, "syscall", &current->location);
    
    node->left = parse_syscall_params(tokens);
    if (!node->left) {
      parser_error(syscall_token, "Failed to parse syscall parameters");
      free_ast_node(node);
      return NULL;
    }
    
    return node;
  }

  if (current->type == TOKEN_CALL || strcmp(current->value, "call") == 0) {
    Token *call_token = current;
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_CALL, "call", &current->location);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      Token *lparen_token = *tokens;
      *tokens = (*tokens)->next;

      if (*tokens && (*tokens)->type != TOKEN_RPAREN) {
        node->left = parse_expression(tokens);
        if (!node->left) {
          parser_error(*tokens ? *tokens : lparen_token, 
                      "Expected function name or first argument for 'call'");
          free_ast_node(node);
          return NULL;
        }

        while (*tokens && (*tokens)->type == TOKEN_COMMA) {
          Token *comma_token = *tokens;
          *tokens = (*tokens)->next;
          
          ASTNode *arg = parse_expression(tokens);
          if (!arg) {
            parser_error(*tokens ? *tokens : comma_token, 
                        "Expected argument after comma in 'call'");
            free_ast_node(node);
            return NULL;
          }
          
          if (!node->right) {
            node->right = arg;
          } else {
            // Find the last argument in the chain
            ASTNode *last = node->right;
            while (last->next) {
              last = last->next;
            }
            last->next = arg;
            arg->prev = last;
          }
        }
      }

      if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
        *tokens = (*tokens)->next;
      } else {
        parser_error(*tokens ? *tokens : lparen_token, 
                    "Expected ')' after call arguments to match opening parenthesis at line %d, column %d",
                    lparen_token->location.line, lparen_token->location.column);
        free_ast_node(node);
        return NULL;
      }
    } else {
      parser_error(*tokens ? *tokens : call_token, "Expected '(' after 'call'");
      free_ast_node(node);
      return NULL;
    }

    return node;
  }

  return NULL;
}

ASTNode *parse_block(Token **tokens) {
  if (!*tokens) {
    parser_error(NULL, "Unexpected end of input while parsing block");
    return NULL;
  }
  
  if ((*tokens)->type != TOKEN_LBRACE) {
    parser_error(*tokens, "Expected '{' to start block, found '%s'", (*tokens)->value);
    return NULL;
  }

  Token *lbrace_token = *tokens;
  *tokens = (*tokens)->next;
  ASTNode *head = NULL;
  ASTNode *tail = NULL;

  while (*tokens && (*tokens)->type != TOKEN_RBRACE) {
    ASTNode *stmt = parse_statement(tokens);
    if (!stmt) {
      parser_error(*tokens ? *tokens : lbrace_token, 
                  "Invalid statement inside block starting at line %d, column %d",
                  lbrace_token->location.line, lbrace_token->location.column);
      
      if (*tokens) {
        parser_error(*tokens, "Offending token: %s (Type: %s)", 
                    (*tokens)->value, 
                    (*tokens)->type < TOKEN_UNKNOWN ? token_type_strings[(*tokens)->type] : "UNKNOWN");
      }
      
      // Clean up any statements we've already parsed
      free_ast_node(head);
      return NULL;
    }

    if (!head)
      head = tail = stmt;
    else {
      tail->next = stmt;
      stmt->prev = tail;
      tail = stmt;
    }

    if (*tokens && (*tokens)->type == TOKEN_SEMICOLON) {
      *tokens = (*tokens)->next;
    } else {
      parser_error(*tokens ? *tokens : lbrace_token, 
                  "Expected ';' after statement at line %d, column %d",
                  tail->location.line, tail->location.column);
      free_ast_node(head);
      return NULL;
    }
  }

  if (*tokens && (*tokens)->type == TOKEN_RBRACE) {
    *tokens = (*tokens)->next; 
  } else {
    parser_error(*tokens ? *tokens : lbrace_token, 
                "Expected '}' to close block starting at line %d, column %d",
                lbrace_token->location.line, lbrace_token->location.column);
    free_ast_node(head);
    return NULL;
  }

  ASTNode *block_node = new_ast_node(TOKEN_LBRACE, "{", &lbrace_token->location);
  block_node->left = head;
  return block_node;
}

ASTNode *parse_label(Token **tokens) {
  if (!tokens || !*tokens)
    return NULL;

  if (!((*tokens)->type == TOKEN_LABEL_KW)) {
    return NULL;
  }

  Token *func_token = *tokens;
  *tokens = (*tokens)->next; 

  if (!*tokens) {
    parser_error(func_token, "Unexpected end of input after 'func' keyword");
    return NULL;
  }
  
  if ((*tokens)->type != TOKEN_LABEL && (*tokens)->type != TOKEN_IDENTIFIER) {
    parser_error(*tokens, "Expected function name after 'func', found '%s'", (*tokens)->value);
    return NULL;
  }

  Token *name_token = *tokens;
  ASTNode *label_node = new_ast_node(TOKEN_LABEL, (*tokens)->value, &name_token->location);
  *tokens = (*tokens)->next;

  ASTNode *block = parse_block(tokens);
  if (!block) {
    parser_error(name_token, "Failed to parse function block for '%s'", name_token->value);
    free_ast_node(label_node);
    return NULL;
  }

  label_node->left = block;
  return label_node;
}

ASTNode *parse_all(Token **tokens) {
  ASTNode *head = NULL;
  ASTNode *tail = NULL;

  while (*tokens) {
    ASTNode *node = NULL;

    if ((*tokens)->type == TOKEN_LABEL_KW) {
      node = parse_label(tokens);
      if (!node) {
        parser_error(*tokens, "Failed to parse function declaration");
        error_exit("Compilation aborted due to syntax errors");
      }
    } else {
      node = parse_statement(tokens);

      if (node) {
        if (*tokens && (*tokens)->type == TOKEN_SEMICOLON) {
          *tokens = (*tokens)->next;
        } else {
          parser_error(*tokens ? *tokens : NULL, 
                      "Expected ';' after statement at line %d, column %d",
                      node->location.line, node->location.column);
          free_ast_node(node);
          free_ast_node(head);
          error_exit("Compilation aborted due to syntax errors");
        }
      }
    }

    if (!node) {
      if (*tokens) {
        parser_error(*tokens, "Unexpected token: %s (Type: %s)", 
                    (*tokens)->value,
                    (*tokens)->type < TOKEN_UNKNOWN ? token_type_strings[(*tokens)->type] : "UNKNOWN");
        // Skip the problematic token to try to continue parsing
        *tokens = (*tokens)->next;
      } else {
        // No more tokens
        break;
      }
      continue;
    }

    if (!head)
      head = tail = node;
    else {
      tail->next = node;
      node->prev = tail;
      tail = node;
    }
  }

  // Connect prev pointers for easy traversal
  connect_prev_pointers(head);
  
  return head;
}
