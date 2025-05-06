#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *new_ast_node(TokenType type, const char *value) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
  node->type = type;
  node->value = strdup(value);
  node->left = NULL;
  node->right = NULL;
  node->next = NULL;
  node->prev = NULL;
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
    return new_ast_node(current->type, current->value);
  }

  if (current->type == TOKEN_LPAREN) {
    *tokens = current->next;
    ASTNode *expr = parse_expression(tokens);
    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected ')'.\n");
    }
    return expr;
  }

  if (current->type == TOKEN_COMMA) {
    *tokens = current->next;
    return parse_expression(tokens);
  }

  fprintf(stderr, "Unexpected expression: %s\n", current->value);
  return NULL;
}

ASTNode *parse_syscall_params(Token **tokens) {
  if (!*tokens || (*tokens)->type != TOKEN_LPAREN) {
    fprintf(stderr, "Expected '(' after syscall.\n");
    return NULL;
  }

  *tokens = (*tokens)->next;

  ASTNode *params_head = NULL;
  ASTNode *params_tail = NULL;

  if (*tokens && (*tokens)->type != TOKEN_RPAREN) {
    ASTNode *param = parse_expression(tokens);
    if (!param) {
      fprintf(stderr, "Expected parameter in syscall.\n");
      return NULL;
    }

    params_head = params_tail = param;

    while (*tokens && (*tokens)->type == TOKEN_COMMA) {
      *tokens = (*tokens)->next;

      param = parse_expression(tokens);
      if (!param) {
        fprintf(stderr, "Expected parameter after comma in syscall.\n");
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
    fprintf(stderr, "Expected ')' after syscall parameters.\n");
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
    *tokens = current->next;

    ASTNode *node = new_ast_node(op_type, current->value);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected '('.\n");
      return NULL;
    }

    node->left = parse_expression(tokens);

    if (*tokens && (*tokens)->type == TOKEN_COMMA) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected ','.\n");
      return NULL;
    }

    node->right = parse_expression(tokens);

    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected ')'.\n");
      return NULL;
    }

    return node;
  }

  if (current->type == TOKEN_JUMP || current->type == TOKEN_JUMP_EQUAL) {
    TokenType op_type = current->type;
    *tokens = current->next;

    ASTNode *node = new_ast_node(op_type, current->value);

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected '('.\n");
      return NULL;
    }

    node->left = parse_expression(tokens);

    if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected ')'.\n");
      return NULL;
    }

    return node;
  }

  if (current->type == TOKEN_RETURN || strcmp(current->value, "return") == 0) {
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_RETURN, "return");

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;

      if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
        *tokens = (*tokens)->next;
      } else {
        fprintf(stderr, "Expected ')' after return.\n");
        return NULL;
      }
    }

    return node;
  }

  if (current->type == TOKEN_SYS_CALL ||
      strcmp(current->value, "syscall") == 0) {
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_SYS_CALL, "syscall");
    node->left = parse_syscall_params(tokens);
    return node;
  }

  if (current->type == TOKEN_CALL || strcmp(current->value, "call") == 0) {
    *tokens = current->next;
    ASTNode *node = new_ast_node(TOKEN_CALL, "call");

    if (*tokens && (*tokens)->type == TOKEN_LPAREN) {
      *tokens = (*tokens)->next;

      if (*tokens && (*tokens)->type != TOKEN_RPAREN) {
        node->left = parse_expression(tokens);

        while (*tokens && (*tokens)->type == TOKEN_COMMA) {
          *tokens = (*tokens)->next;
          node->right = parse_expression(tokens);
        }
      }

      if (*tokens && (*tokens)->type == TOKEN_RPAREN) {
        *tokens = (*tokens)->next;
      } else {
        fprintf(stderr, "Expected ')' after call.\n");
        return NULL;
      }
    }

    return node;
  }

  return NULL;
}

ASTNode *parse_block(Token **tokens) {
  if (!*tokens || (*tokens)->type != TOKEN_LBRACE) {
    fprintf(stderr, "Expected '{'.\n");
    return NULL;
  }

  *tokens = (*tokens)->next;
  ASTNode *head = NULL;
  ASTNode *tail = NULL;

  while (*tokens && (*tokens)->type != TOKEN_RBRACE) {
    ASTNode *stmt = parse_statement(tokens);
    if (!stmt) {
      fprintf(stderr, "Invalid statement inside block.\n");
      if (*tokens) {
        fprintf(stderr, "Offending token: %s (Type: %d)\n", (*tokens)->value,
                (*tokens)->type);
      }
      break;
    }

    if (!head)
      head = tail = stmt;
    else {
      tail->next = stmt;
      tail = stmt;
    }

    if (*tokens && (*tokens)->type == TOKEN_SEMICOLON) {
      *tokens = (*tokens)->next;
    } else {
      fprintf(stderr, "Expected ';' after statement.\n");
    }
  }

  if (*tokens && (*tokens)->type == TOKEN_RBRACE) {
    *tokens = (*tokens)->next; 
  } else {
    fprintf(stderr, "Expected '}'.\n");
    return NULL;
  }

  ASTNode *block_node = new_ast_node(TOKEN_LBRACE, "{");
  block_node->left = head;
  return block_node;
}

ASTNode *parse_label(Token **tokens) {
  if (!tokens || !*tokens)
    return NULL;

  if (!((*tokens)->type == TOKEN_LABEL_KW)) {
    return NULL;
  }

  *tokens = (*tokens)->next; 

  if (!*tokens ||
      ((*tokens)->type != TOKEN_LABEL && (*tokens)->type != TOKEN_IDENTIFIER)) {
    fprintf(stderr, "Expected function name.\n");
    return NULL;
  }

  ASTNode *label_node = new_ast_node(TOKEN_LABEL, (*tokens)->value);
  *tokens = (*tokens)->next;

  ASTNode *block = parse_block(tokens);
  if (!block) {
    fprintf(stderr, "Failed to parse function block.\n");
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
    } else {
      node = parse_statement(tokens);

      if (*tokens && (*tokens)->type == TOKEN_SEMICOLON) {
        *tokens = (*tokens)->next;
      }
    }

    if (!node) {
      fprintf(stderr, "General parsing error.\n");
      if (*tokens) {
        fprintf(stderr, "Current token: %s (Type: %d)\n", (*tokens)->value,
                (*tokens)->type);
        *tokens = (*tokens)->next; 
      } else {
        break; 
      }
      continue; 
    }

    if (!head)
      head = tail = node;
    else {
      tail->next = node;
      tail = node;
    }
  }

  return head;
}
