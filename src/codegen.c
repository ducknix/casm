#include "codegen.h"
#include "lexer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int string_counter = 0;
int return_label_counter = 0;  // Counter for unique return labels

typedef struct {
    char *label;         
    int length;          
    char *string_value;  
} StringInfo;

#define MAX_STRINGS 100
StringInfo string_infos[MAX_STRINGS];
int string_info_count = 0;

// Track function calls to generate proper return labels
typedef struct {
    char *caller_func;   // Name of the calling function
    char *callee_func;   // Name of the called function
    char *return_label;  // Generated return label
    int used;           // Flag to mark if this return label has been used
} FunctionCall;

#define MAX_CALLS 100
FunctionCall function_calls[MAX_CALLS];
int function_call_count = 0;

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

char *add_string_info(const char *string_value) {
    if (string_info_count >= MAX_STRINGS) {
        fprintf(stderr, "Too many strings in the program!\n");
        return NULL;
    }

    char *label = malloc(32);
    sprintf(label, "str_%d", string_counter++);

    int length = calculate_string_length(string_value);

    string_infos[string_info_count].label = strdup(label);
    string_infos[string_info_count].length = length;
    string_infos[string_info_count].string_value = strdup(string_value);
    string_info_count++;

    return label;
}

int find_string_length(const char *label) {
    for (int i = 0; i < string_info_count; i++) {
        if (strcmp(string_infos[i].label, label) == 0) {
            return string_infos[i].length;
        }
    }
    return 0; 
}

int find_last_string_index(ASTNode *node) {
    if (!node) return -1;

    int last_string_index = -1;

    ASTNode *param = node;
    int index = 0;

    while (param) {
        if (param->type == TOKEN_STRING || param->type == TOKEN_LABEL) {
            last_string_index = index;
        }
        param = param->next;
        index++;
    }

    return last_string_index;
}

char *process_string_literal(const char *string_value, FILE *fp) {
  char *label = add_string_info(string_value);

  fprintf(fp, "    %s db %s, 0\n", label, string_value);

  return label;
}

void handle_move(ASTNode *move_node, FILE *fp) {
  if (!move_node->left || !move_node->right)
    return;

  if (move_node->right->type == TOKEN_STRLEN) {

    ASTNode *current = move_node->prev;
    const char *last_string_label = NULL;

    while (current) {
        if (current->type == TOKEN_MOVE && current->right && 
            (current->right->type == TOKEN_STRING || current->right->type == TOKEN_LABEL)) {
            last_string_label = current->right->value;
            break;
        }
        current = current->prev;
    }

    if (last_string_label) {
        int length = find_string_length(last_string_label);
        fprintf(fp, "    mov %s, %d\n", translate_register(move_node->left->value), length);
    } else {
        fprintf(stderr, "Warning: No previous string found for strlen\n");
        fprintf(fp, "    mov %s, 0\n", translate_register(move_node->left->value));
    }
    return;
  }

  if (move_node->right->type == TOKEN_NUMBER) {
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value), move_node->right->value);
  } else if (move_node->right->type == TOKEN_STRING) {
    char *label = process_string_literal(move_node->right->value, fp);
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value), label);
    free(label);
  } else {
    fprintf(fp, "    mov %s, %s\n", translate_register(move_node->left->value), 
            translate_register(move_node->right->value));
  }
}

#define REG_MAP(index) (index < 7 ? reg_map[index] : "invalid_register")

void process_syscall_parameters(ASTNode *syscall_node, FILE *fp) {
  ASTNode *params = syscall_node->left;
  if (!params)
    return;

  ASTNode *param_array[7] = {NULL}; 
  int param_count = 0;
  int string_param_index = -1;

  ASTNode *current = params;
  while (current && param_count < 7) {
    param_array[param_count] = current;

    if (current->type == TOKEN_STRING) {
      char *label = process_string_literal(current->value, fp);
      free(current->value);
      current->value = strdup(label);
      current->type = TOKEN_LABEL;
      string_param_index = param_count;
      free(label);
    }

    param_count++;
    current = current->next;
  }

  for (int i = 0; i < param_count; i++) {
    if (param_array[i] && param_array[i]->type == TOKEN_STRLEN) {

      int found_string_index = -1;
      for (int j = 0; j < param_count; j++) {
        if (param_array[j] && param_array[j]->type == TOKEN_LABEL) {

          if (strncmp(param_array[j]->value, "str_", 4) == 0) {
            found_string_index = j;

            string_param_index = j;
          }
        }
      }

      if (string_param_index != -1) {
        char *string_label = param_array[string_param_index]->value;
        int length = find_string_length(string_label);

        free(param_array[i]->value);
        char tmp[16];
        sprintf(tmp, "%d", length);
        param_array[i]->value = strdup(tmp);
        param_array[i]->type = TOKEN_NUMBER;
      } else {

        fprintf(stderr, "Warning: No string parameter found for syscall with strlen\n");
        free(param_array[i]->value);
        param_array[i]->value = strdup("0");
        param_array[i]->type = TOKEN_NUMBER;
      }
    }
  }

  const char *reg_map[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"};

  for (int i = 0; i < param_count; i++) {
    if (param_array[i]) {
      if (param_array[i]->type == TOKEN_NUMBER) {
        fprintf(fp, "    mov %s, %s\n", REG_MAP(i), param_array[i]->value);
      } else if (param_array[i]->type == TOKEN_LABEL) {
        fprintf(fp, "    mov %s, %s\n", REG_MAP(i), param_array[i]->value);
      } else {
        fprintf(fp, "    mov %s, %s\n", REG_MAP(i), translate_register(param_array[i]->value));
      }
    }
  }

  fprintf(fp, "    int 0x80\n");
}

// Function to find an existing call or create a new one
// Returns the index of the call in the function_calls array
int register_function_call(const char* caller_func, const char* callee_func) {
    // First check if we already have this call registered
    for (int i = 0; i < function_call_count; i++) {
        if (strcmp(function_calls[i].caller_func, caller_func) == 0 && 
            strcmp(function_calls[i].callee_func, callee_func) == 0) {
            return i; // We already have this call registered
        }
    }
    
    // If not, create a new entry
    if (function_call_count >= MAX_CALLS) {
        fprintf(stderr, "Too many function calls!\n");
        return -1;
    }
    
    char *label = malloc(64);
    sprintf(label, "__backto_%s_%d", caller_func, function_call_count);
    
    function_calls[function_call_count].caller_func = strdup(caller_func);
    function_calls[function_call_count].callee_func = strdup(callee_func);
    function_calls[function_call_count].return_label = strdup(label);
    function_calls[function_call_count].used = 0;
    
    free(label);
    return function_call_count++;
}

// Generate a unique return label for function calls
char* generate_return_label(const char* caller_func, const char* callee_func) {
    int call_index = register_function_call(caller_func, callee_func);
    if (call_index == -1) {
        return strdup("__error_label");
    }
    
    return strdup(function_calls[call_index].return_label);
}

void generate_block(FILE *fp, ASTNode *node, char **data_section_strings,
                    const char *func_name) {
  ASTNode *current = node->left;

  while (current != NULL) {
    switch (current->type) {
    case TOKEN_MOVE:
      if (current->left && current->right) {
        handle_move(current, fp);
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

      case TOKEN_JUMP_NOT_EQUAL:
      if (current->left) {
        fprintf(fp, "    jne %s\n", current->left->value);
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
        // Generate a unique return label for this function call
        char* return_label = generate_return_label(func_name, current->left->value);
        
        // Jump to the target function
        fprintf(fp, "    jmp %s\n", current->left->value);
        
        // Define the return label
        fprintf(fp, "%s:\n", return_label);
        
        free(return_label);
      } else {
        // Original implementation for syscalls
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

void collect_strings(ASTNode *ast, FILE *fp) {
  fprintf(fp, "section .data\n");

  string_counter = 0;
  string_info_count = 0;

  ASTNode *current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      ASTNode *block = current->left;
      if (block && block->type == TOKEN_LBRACE) {
        ASTNode *stmt = block->left;
        while (stmt != NULL) {
          if (stmt->type == TOKEN_MOVE && stmt->right &&
              stmt->right->type == TOKEN_STRING) {
            char *str_value = stmt->right->value;
            size_t len = strlen(str_value);
            if (len >= 2 && str_value[0] == '"' && str_value[len - 1] == '"') {
              char *label = add_string_info(str_value);
              fprintf(fp, "    %s db %s, 0\n", label, str_value);

              free(stmt->right->value);
              stmt->right->value = strdup(label);
              stmt->right->type = TOKEN_LABEL;
              free(label);
            }
          }
          else if (stmt->type == TOKEN_SYS_CALL) {
            ASTNode *param = stmt->left;
            while (param) {
              if (param->type == TOKEN_STRING) {
                char *str_value = param->value;
                size_t len = strlen(str_value);
                if (len >= 2 && str_value[0] == '"' && str_value[len - 1] == '"') {
                  char *label = add_string_info(str_value);
                  fprintf(fp, "    %s db %s, 0\n", label, str_value);

                  free(param->value);
                  param->value = strdup(label);
                  param->type = TOKEN_LABEL;
                  free(label);
                }
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

// Find the exact return label for a function call
const char* find_return_label(const char* caller, const char* callee) {
    for (int i = 0; i < function_call_count; i++) {
        if (strcmp(function_calls[i].caller_func, caller) == 0 && 
            strcmp(function_calls[i].callee_func, callee) == 0) {
            function_calls[i].used = 1; // Mark this label as used
            return function_calls[i].return_label;
        }
    }
    return NULL;
}

void cleanup_string_infos() {
    for (int i = 0; i < string_info_count; i++) {
        free(string_infos[i].label);
        free(string_infos[i].string_value);
    }
    string_info_count = 0;
}

void cleanup_function_calls() {
    for (int i = 0; i < function_call_count; i++) {
        free(function_calls[i].caller_func);
        free(function_calls[i].callee_func);
        free(function_calls[i].return_label);
    }
    function_call_count = 0;
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

  connect_prev_pointers(ast);

  // Reset counters and arrays
  function_call_count = 0;

  // First pass to pre-register all function calls
  ASTNode *current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      const char* func_name = current->value;
      
      if (current->left && current->left->type == TOKEN_LBRACE) {
        ASTNode *stmt = current->left->left;
        while (stmt != NULL) {
          if (stmt->type == TOKEN_CALL && stmt->left) {
            // Register this function call
            register_function_call(func_name, stmt->left->value);
          }
          stmt = stmt->next;
        }
      }
    }
    current = current->next;
  }

  collect_strings(ast, fp);

  fprintf(fp, "section .text\n");
  fprintf(fp, "global _start\n\n");

  WRITE_EXIT_SECTION(fp);

  current = ast;
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

      // End of function handling
      if (strcmp(current->value, "main") == 0) {
        fprintf(fp, "    jmp _exit\n");
      } else {
        // Find all functions that call this function
        bool returnWritten = false;
        for (int i = 0; i < function_call_count; i++) {
            if (strcmp(function_calls[i].callee_func, current->value) == 0 && !function_calls[i].used) {
                fprintf(fp, "    jmp %s\n", function_calls[i].return_label);
                function_calls[i].used = 1; // Mark as used
                returnWritten = true;
                break;
            }
        }
        
        // If no function calls this function, add a return instruction
        if (!returnWritten) {
            fprintf(fp, "    ret\n");
        }
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

  fclose(fp);
  cleanup_string_infos();
  cleanup_function_calls();

  printf("NASM code successfully generated: %s\n", output_file);
}
