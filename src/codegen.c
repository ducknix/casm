#include "codegen.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int string_counter = 0;

// Makrolar ve yardımcı fonksiyonlar
#define REGISTER_MAP(X) \
  X("&1", "eax") \
  X("&2", "ebx") \
  X("&3", "ecx") \
  X("&4", "edx") \
  X("&5", "esi") \
  X("&6", "edi") \
  X("&7", "ebp")

const char *translate_register(const char *reg) {
  #define CHECK_REG(src, dst) if (strcmp(reg, src) == 0) return dst;
  REGISTER_MAP(CHECK_REG)
  #undef CHECK_REG
  return reg;
}

#define GENERATE_BINARY_OP(fp, op, left, right) do { \
  if ((right)->type == TOKEN_NUMBER) { \
    fprintf((fp), "    %s %s, %s\n", (op), translate_register((left)->value), (right)->value); \
  } else { \
    fprintf((fp), "    %s %s, %s\n", (op), translate_register((left)->value), \
            translate_register((right)->value)); \
  } \
} while(0)

int calculate_string_length(const char *string_value) {
  if (!string_value)
    return 0;

  size_t len = strlen(string_value);
  int actual_len = 0;

  if (len >= 2 && string_value[0] == '"' && string_value[len - 1] == '"') {
    int i = 1;
    while (i < len - 1) {
      if (string_value[i] == '\\') {
        i++;
        if (i < len - 1) {
          actual_len++;
          i++;
        }
      } else {
        actual_len++;
        i++;
      }
    }
  } else {
    actual_len = len;
  }

  return actual_len;
}

char *process_string_literal(const char *string_value, FILE *fp) {
  char *label = malloc(32); 
  sprintf(label, "str_%d", string_counter++);

  fprintf(fp, "    %s db %s, 0\n", label, string_value);

  return label;
}

typedef struct {
  char *value;    
  char *label;    
  int context_id; 
} StringEntry;

#define MAX_STRINGS 100

StringEntry string_table[MAX_STRINGS];
int string_table_count = 0;
int current_context_id = 0;

void add_string_to_table(const char *value, const char *label) {
  if (string_table_count < MAX_STRINGS) {
    string_table[string_table_count].value = strdup(value);
    string_table[string_table_count].label = strdup(label);
    string_table[string_table_count].context_id = current_context_id;
    string_table_count++;
  }
}

void start_new_string_context() { 
  current_context_id++; 
}

const char *find_string_in_context(int context_id) {
  for (int i = string_table_count - 1; i >= 0; i--) {
    if (string_table[i].context_id == context_id) {
      return string_table[i].value;
    }
  }

  if (context_id > 0) {
    for (int i = string_table_count - 1; i >= 0; i--) {
      return string_table[i].value;
    }
  }

  return NULL;
}

#define SAFE_STRDUP(ptr, val) do { \
  free(ptr); \
  ptr = strdup(val); \
} while(0)

void handle_move_with_strlen(ASTNode *move_node, FILE *fp) {
  if (move_node->left && move_node->right &&
      move_node->right->type == TOKEN_STRLEN) {

    int move_context_id = current_context_id;
    const char *string_value = NULL;

    ASTNode *current_node = move_node->prev;
    while (current_node) {
      if (current_node->type == TOKEN_MOVE && current_node->right &&
          current_node->right->type == TOKEN_STRING) {
        string_value = current_node->right->value;
        break;
      } else if (current_node->type == TOKEN_SYS_CALL && current_node->left) {
        ASTNode *param = current_node->left;
        while (param) {
          if (param->type == TOKEN_STRING) {
            string_value = param->value;
            break;
          }
          param = param->next;
        }
        if (string_value)
          break;
      }
      current_node = current_node->prev;
    }

    if (!string_value) {
      string_value = find_string_in_context(move_context_id);
    }

    if (!string_value && string_table_count > 0) {
      string_value = string_table[string_table_count - 1].value;
    }

    if (string_value) {
      int str_len = calculate_string_length(string_value);
      fprintf(fp, "    mov %s, %d\n",
              translate_register(move_node->left->value), str_len);
      return;
    } else {
      fprintf(stderr, "Warning: No string found for string length token, "
                      "defaulting to length 0\n");
      fprintf(fp, "    mov %s, 0\n",
              translate_register(move_node->left->value));
      return;
    }
  }

  if (move_node->right->type == TOKEN_NUMBER) {
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value),
            move_node->right->value);
  } else if (move_node->right->type == TOKEN_STRING) {
    start_new_string_context();

    char *label = process_string_literal(move_node->right->value, fp);
    add_string_to_table(move_node->right->value, label);
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value),
            label);
    free(label);
  } else {
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value),
            translate_register(move_node->right->value));
  }
}

#define REG_MAP(index) (index < 7 ? reg_map[index] : "invalid_register")

void process_syscall_parameters(ASTNode *syscall_node, FILE *fp) {
  start_new_string_context();

  ASTNode *params = syscall_node->left;
  if (!params)
    return;

  ASTNode *param_array[7] = {NULL}; 
  int param_count = 0;

  ASTNode *current = params;
  while (current && param_count < 7) {
    param_array[param_count] = current;

    if (current->type == TOKEN_STRING) {
      char *label = process_string_literal(current->value, fp);
      add_string_to_table(current->value, label);
      free(label);
    }

    param_count++;
    current = current->next;
  }

  #define HANDLE_STRLEN_PARAM(param_index) do { \
    const char *string_value = NULL; \
    for (int j = param_index - 1; j >= 0; j--) { \
      if (param_array[j] && param_array[j]->type == TOKEN_STRING) { \
        string_value = param_array[j]->value; \
        break; \
      } \
    } \
    if (!string_value) { \
      string_value = find_string_in_context(current_context_id); \
    } \
    if (string_value) { \
      int str_len = calculate_string_length(string_value); \
      free(param_array[param_index]->value); \
      param_array[param_index]->value = malloc(16); \
      sprintf(param_array[param_index]->value, "%d", str_len); \
      param_array[param_index]->type = TOKEN_NUMBER; \
    } else { \
      fprintf(stderr, "Warning: No string found for string length token, " \
                      "defaulting to length 0\n"); \
      free(param_array[param_index]->value); \
      param_array[param_index]->value = strdup("0"); \
      param_array[param_index]->type = TOKEN_NUMBER; \
    } \
  } while(0)

  for (int i = 0; i < param_count; i++) {
    if (param_array[i] && param_array[i]->type == TOKEN_STRLEN) {
      HANDLE_STRLEN_PARAM(i);
    }
  }
  #undef HANDLE_STRLEN_PARAM

  const char *reg_map[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"};
  
  #define SET_PARAM_TO_REG(param, reg_index) do { \
    if (param->type == TOKEN_NUMBER) { \
      fprintf(fp, "    mov %s, %s\n", REG_MAP(reg_index), param->value); \
    } else if (param->type == TOKEN_STRING) { \
      char *label = NULL; \
      for (int j = 0; j < string_table_count; j++) { \
        if (strcmp(string_table[j].value, param->value) == 0 && \
            string_table[j].context_id == current_context_id) { \
          label = strdup(string_table[j].label); \
          break; \
        } \
      } \
      if (!label) { \
        label = process_string_literal(param->value, fp); \
      } \
      fprintf(fp, "    mov %s, %s\n", REG_MAP(reg_index), label); \
      free(label); \
    } else if (param->type == TOKEN_LABEL) { \
      fprintf(fp, "    mov %s, %s\n", REG_MAP(reg_index), param->value); \
    } else { \
      fprintf(fp, "    mov %s, %s\n", REG_MAP(reg_index), \
              translate_register(param->value)); \
    } \
  } while(0)

  for (int i = 0; i < param_count; i++) {
    SET_PARAM_TO_REG(param_array[i], i);
  }
  #undef SET_PARAM_TO_REG

  fprintf(fp, "    int 0x80\n");
}

void generate_block(FILE *fp, ASTNode *node, char **data_section_strings,
                    const char *func_name) {
  ASTNode *current = node->left; 

  int data_section_count = 0; 

  if (current) {
    ASTNode *prev = NULL;
    ASTNode *temp = current;
    while (temp) {
      temp->prev = prev; 
      prev = temp;
      temp = temp->next;
    }
  }

  current_context_id = 0;

  while (current != NULL) {
    switch (current->type) {
    case TOKEN_MOVE:
      if (current->left && current->right) {
        start_new_string_context();
        handle_move_with_strlen(current, fp);
      }
      break;

    case TOKEN_ADD:
      if (current->left && current->right) {
        GENERATE_BINARY_OP(fp, "add", current->left, current->right);
      }
      break;

    case TOKEN_SUB:
      if (current->left && current->right) {
        GENERATE_BINARY_OP(fp, "sub", current->left, current->right);
      }
      break;

    case TOKEN_COMPARE:
      if (current->left && current->right) {
        GENERATE_BINARY_OP(fp, "cmp", current->left, current->right);
      }
      break;

    case TOKEN_JUMP:
      if (current->left) {
        fprintf(fp, "    jmp %s\n", current->left->value);
      }
      break;

    case TOKEN_JUMP_EQUAL:
      if (current->left) {
        fprintf(fp, "    je %s\n", current->left->value);
      }
      break;

    case TOKEN_RETURN:
      if (strcmp(func_name, "main") == 0) {
        fprintf(fp, "    jmp _exit\n");
      } else {
        fprintf(fp, "    ret\n");
      }
      break;

    case TOKEN_CALL:
      if (current->left) {
        fprintf(fp, "    int %s\n", current->left->value);
      } else {
        fprintf(fp, "    int 0x80\n");
      }
      break;

    case TOKEN_SYS_CALL:
      process_syscall_parameters(current, fp);
      break;

    default:
      fprintf(stderr, "Unknown AST node type: %d\n", current->type);
      break;
    }

    current = current->next;
  }
}

#define CLEANUP_STRING_TABLE() do { \
  for (int i = 0; i < string_table_count; i++) { \
    free(string_table[i].value); \
    free(string_table[i].label); \
  } \
  string_table_count = 0; \
  string_counter = 0; \
  current_context_id = 0; \
} while(0)

#define PROCESS_STRING_NODE(node, fp) do { \
  char *str_value = node->value; \
  size_t len = strlen(str_value); \
  if (len >= 2 && str_value[0] == '"' && str_value[len - 1] == '"') { \
    char label[32]; \
    sprintf(label, "str_%d", string_counter++); \
    fprintf(fp, "    %s db %s, 0\n", label, str_value); \
    add_string_to_table(str_value, label); \
    free(node->value); \
    node->value = strdup(label); \
    node->type = TOKEN_LABEL; \
  } \
} while(0)

void collect_strings(ASTNode *ast, FILE *fp) {
  fprintf(fp, "section .data\n");
  
  CLEANUP_STRING_TABLE();

  ASTNode *current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      ASTNode *block = current->left;
      if (block && block->type == TOKEN_LBRACE) {
        ASTNode *stmt = block->left;
        while (stmt != NULL) {
          if (stmt->type == TOKEN_MOVE && stmt->right &&
              stmt->right->type == TOKEN_STRING) {
            PROCESS_STRING_NODE(stmt->right, fp);
          }
          else if (stmt->type == TOKEN_SYS_CALL) {
            start_new_string_context();
            
            ASTNode *param = stmt->left;
            while (param) {
              if (param->type == TOKEN_STRING) {
                PROCESS_STRING_NODE(param, fp);
              }
              param = param->next;
            }
          }
          stmt = stmt->next;
        }
      }
    }
    current = current->next;
  }

  fprintf(fp, "\n");
}

#define WRITE_EXIT_SECTION(fp) do { \
  fprintf(fp, "_exit:\n"); \
  fprintf(fp, "    mov eax, 1      ; exit system call\n"); \
  fprintf(fp, "    xor ebx, ebx    ; exit code 0\n"); \
  fprintf(fp, "    int 0x80        ; call kernel\n\n"); \
} while(0)

void generate_nasm(ASTNode *ast, const char *output_file) {
  FILE *fp = fopen(output_file, "w");

  if (!fp) {
    fprintf(stderr, "Failed to open output file: %s\n", output_file);
    return;
  }

  collect_strings(ast, fp);

  fprintf(fp, "section .text\n");
  fprintf(fp, "global _start\n\n");

  WRITE_EXIT_SECTION(fp);

  ASTNode *current = ast;
  bool found_main = false;

  while (current != NULL) {
    if (current->type == TOKEN_LABEL && strcmp(current->value, "main") == 0) {
      found_main = true;
      break;
    }
    current = current->next;
  }

  if (found_main) {
    fprintf(fp, "_start:\n");
    fprintf(fp, "    jmp main     ; Call the main function\n\n");
  }

  current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      fprintf(fp, "%s:\n", current->value);

      if (current->left && current->left->type == TOKEN_LBRACE) {
        char *data_strings[100]; 
        generate_block(fp, current->left, data_strings, current->value);
      }

      if (strcmp(current->value, "main") == 0) {
        fprintf(fp, "    jmp _exit\n");
      } else {
        fprintf(fp, "    ret\n");
      }

      fprintf(fp, "\n");
    }

    current = current->next;
  }

  if (!found_main) {
    fprintf(fp, "_start:\n");
    fprintf(fp, "    ; No main function found, exiting directly\n");
    fprintf(fp, "    jmp _exit\n");
  }

  CLEANUP_STRING_TABLE();

  fclose(fp);

  printf("NASM code successfully generated: %s\n", output_file);
}
