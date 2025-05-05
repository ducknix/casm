#include "codegen.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Counter for generating unique labels for strings.
 */
int string_counter = 0;

/**
 * @brief Translates a register name into its corresponding x86 register.
 *
 * @param reg The register name (e.g., "a", "b", etc.).
 * @return The corresponding x86 register name (e.g., "eax", "ebx", etc.), or
 * the original string if no translation is available.
 */
const char *translate_register(const char *reg) {
  if (strcmp(reg, "a") == 0)
    return "eax";
  if (strcmp(reg, "b") == 0)
    return "ebx";
  if (strcmp(reg, "c") == 0)
    return "ecx";
  if (strcmp(reg, "d") == 0)
    return "edx";
  return reg;
}

/**
 * @brief Calculates the length of a string, considering escape sequences and
 * surrounding quotes.
 *
 * @param string_value The string whose length is to be calculated.
 * @return The length of the string, excluding escape characters and quotes.
 */
int calculate_string_length(const char *string_value) {
  if (!string_value)
    return 0;

  size_t len = strlen(string_value);
  int actual_len = 0;

  // Handle strings with surrounding quotes
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

/**
 * @brief Processes a string literal and adds it to the data section.
 *
 * @param string_value The string literal to be processed.
 * @param fp The file pointer to write the NASM code.
 * @return The label for the string in the data section.
 */
char *process_string_literal(const char *string_value, FILE *fp) {
  char *label = malloc(32); // Allocate space for "str_" + number
  sprintf(label, "str_%d", string_counter++);

  // Add string to the data section in NASM format
  fprintf(fp, "    %s db %s, 0\n", label, string_value);

  return label;
}

/**
 * @brief Structure to represent a string entry in the string table.
 */
typedef struct {
  char *value;    /**< The actual string value. */
  char *label;    /**< The label for the string in the data section. */
  int context_id; /**< The context ID to track where the string belongs. */
} StringEntry;

/**
 * @brief Maximum number of strings to store in the string table.
 */
#define MAX_STRINGS 100

/**
 * @brief The global string table to store string literals and their labels.
 */
StringEntry string_table[MAX_STRINGS];

/**
 * @brief The number of strings currently in the string table.
 */
int string_table_count = 0;

/**
 * @brief The context ID used to track string usage within different code
 * blocks.
 */
int current_context_id = 0;

/**
 * @brief Adds a string to the global string table.
 *
 * @param value The value of the string to add.
 * @param label The label of the string in the data section.
 */
void add_string_to_table(const char *value, const char *label) {
  if (string_table_count < MAX_STRINGS) {
    string_table[string_table_count].value = strdup(value);
    string_table[string_table_count].label = strdup(label);
    string_table[string_table_count].context_id = current_context_id;
    string_table_count++;
  }
}

/**
 * @brief Starts a new string context. This increments the context ID to isolate
 * different string usage.
 */
void start_new_string_context() { current_context_id++; }

/**
 * @brief Finds a string in the current context or any previous contexts.
 *
 * @param context_id The context ID to search for.
 * @return The string value if found, or NULL if not found.
 */
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

/**
 * @brief Handles a move statement that involves a string length operation,
 * replacing the string length with its actual value.
 *
 * @param move_node The AST node representing the move statement.
 * @param fp The file pointer to write the NASM code.
 */
void handle_move_with_counter(ASTNode *move_node, FILE *fp) {
  if (move_node->left && move_node->right &&
      move_node->right->type == TOKEN_STRLEN) {

    int move_context_id = current_context_id;
    const char *string_value = NULL;

    // Look for the most recent string in the current block
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

    // Check the string table for the string if not found locally
    if (!string_value) {
      string_value = find_string_in_context(move_context_id);
    }

    // Use the global string table if no string found in the current context
    if (!string_value && string_table_count > 0) {
      string_value = string_table[string_table_count - 1].value;
    }

    // If a string is found, calculate its length and perform the move
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

  // Normal move handling
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

/**
 * @brief Processes system call parameters, handling string literals and
 * `&strlen&` tokens.
 *
 * @param syscall_node The AST node representing the system call.
 * @param fp The file pointer to write the NASM code.
 */
void process_syscall_parameters(ASTNode *syscall_node, FILE *fp) {
  start_new_string_context();

  ASTNode *params = syscall_node->left;
  if (!params)
    return;

  ASTNode *param_array[7] = {NULL}; // Max 7 parameters for syscall
  int param_count = 0;

  // Collect parameters
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

  // Process parameters and handle string length calculations
  for (int i = 0; i < param_count; i++) {
    if (param_array[i] && param_array[i]->type == TOKEN_STRLEN) {
      const char *string_value = NULL;

      // Look in previous parameters for a string
      for (int j = i - 1; j >= 0; j--) {
        if (param_array[j] && param_array[j]->type == TOKEN_STRING) {
          string_value = param_array[j]->value;
          break;
        }
      }

      // Try the string table if not found in parameters
      if (!string_value) {
        string_value = find_string_in_context(current_context_id);
      }

      // Calculate the string length if a string is found
      if (string_value) {
        int str_len = calculate_string_length(string_value);
        free(param_array[i]->value);
        param_array[i]->value = malloc(16);
        sprintf(param_array[i]->value, "%d", str_len);
        param_array[i]->type = TOKEN_NUMBER;
      } else {
        fprintf(stderr, "Warning: No string found for string length token, "
                        "defaulting to length 0\n");
        free(param_array[i]->value);
        param_array[i]->value = strdup("0");
        param_array[i]->type = TOKEN_NUMBER;
      }
    }
  }

  // Move parameters to registers
  const char *reg_map[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"};
  for (int i = 0; i < param_count; i++) {
    ASTNode *param = param_array[i];

    if (param->type == TOKEN_NUMBER) {
      fprintf(fp, "    mov %s, %s\n", reg_map[i], param->value);
    } else if (param->type == TOKEN_STRING) {
      char *label = NULL;

      for (int j = 0; j < string_table_count; j++) {
        if (strcmp(string_table[j].value, param->value) == 0 &&
            string_table[j].context_id == current_context_id) {
          label = strdup(string_table[j].label);
          break;
        }
      }

      if (!label) {
        label = process_string_literal(param->value, fp);
      }

      fprintf(fp, "    mov %s, %s\n", reg_map[i], label);
      free(label);
    } else if (param->type == TOKEN_LABEL) {
      fprintf(fp, "    mov %s, %s\n", reg_map[i], param->value);
    } else {
      fprintf(fp, "    mov %s, %s\n", reg_map[i],
              translate_register(param->value));
    }
  }

  // Perform system call interrupt
  fprintf(fp, "    int 0x80\n");
}

/**
 * @brief Generates the assembly code for a block of statements.
 *
 * This function processes a block of statements from the AST (Abstract Syntax
 * Tree) and generates the corresponding assembly code. It handles various types
 * of statements including MOV, ADD, SUB, and system calls. It also manages
 * string contexts for string literals and handles system call parameters in a
 * structured manner.
 *
 * @param fp              The output file pointer where the generated assembly
 * code will be written.
 * @param node            The AST node representing the block of statements.
 * @param data_section_strings An array of strings used in the data section for
 * string literals.
 * @param func_name       The name of the current function being processed.
 */
void generate_block(FILE *fp, ASTNode *node, char **data_section_strings,
                    const char *func_name) {
  ASTNode *current = node->left; // The left child of the block node contains
                                 // the actual statements.
  int data_section_count =
      0; // Keeps track of how many data section strings are added.

  // Add previous pointers to all nodes in the block for context tracking.
  if (current) {
    ASTNode *prev = NULL;
    ASTNode *temp = current;
    while (temp) {
      temp->prev = prev; // Set the 'prev' pointer of each node for easier
                         // context tracking.
      prev = temp;
      temp = temp->next;
    }
  }

  // Reset context for this block.
  current_context_id = 0;

  // Iterate through each statement in the block and generate code accordingly.
  while (current != NULL) {
    // Handle different types of AST nodes and generate corresponding assembly
    // code.
    switch (current->type) {
    case TOKEN_MOVE:
      if (current->left && current->right) {
        // For MOVE statements, generate assembly code.
        start_new_string_context();            // Start a new context for string
                                               // operations.
        handle_move_with_counter(current, fp); // Handle the move statement.
      }
      break;

    case TOKEN_ADD:
      if (current->left && current->right) {
        // For ADD statements, generate corresponding assembly code.
        if (current->right->type == TOKEN_NUMBER) {
          fprintf(fp, "    add %s, %s\n",
                  translate_register(current->left->value),
                  current->right->value);
        } else {
          fprintf(fp, "    add %s, %s\n",
                  translate_register(current->left->value),
                  translate_register(current->right->value));
        }
      }
      break;

    case TOKEN_SUB:
      if (current->left && current->right) {
        // For SUB statements, generate corresponding assembly code.
        if (current->right->type == TOKEN_NUMBER) {
          fprintf(fp, "    sub %s, %s\n",
                  translate_register(current->left->value),
                  current->right->value);
        } else {
          fprintf(fp, "    sub %s, %s\n",
                  translate_register(current->left->value),
                  translate_register(current->right->value));
        }
      }
      break;

    case TOKEN_COMPARE:
      if (current->left && current->right) {
        // For COMPARE statements, generate corresponding assembly code.
        if (current->right->type == TOKEN_NUMBER) {
          fprintf(fp, "    cmp %s, %s\n",
                  translate_register(current->left->value),
                  current->right->value);
        } else {
          fprintf(fp, "    cmp %s, %s\n",
                  translate_register(current->left->value),
                  translate_register(current->right->value));
        }
      }
      break;

    case TOKEN_JUMP:
      if (current->left) {
        // For JUMP statements, generate a jump instruction.
        fprintf(fp, "    jmp %s\n", current->left->value);
      }
      break;

    case TOKEN_JUMP_EQUAL:
      if (current->left) {
        // For JUMP_EQUAL statements, generate a conditional jump.
        fprintf(fp, "    je %s\n", current->left->value);
      }
      break;

    case TOKEN_RETURN:
      // For RETURN statements, handle return based on function type.
      if (strcmp(func_name, "main") == 0) {
        fprintf(fp, "    jmp _exit\n"); // For 'main', jump to exit code.
      } else {
        fprintf(fp,
                "    ret\n"); // For other functions, use a return instruction.
      }
      break;

    case TOKEN_CALL:
      if (current->left) {
        // For CALL statements, generate a system call.
        fprintf(fp, "    int %s\n", current->left->value);
      } else {
        // If no specific system call is provided, default to the 'int 0x80'
        // system call.
        fprintf(fp, "    int 0x80\n");
      }
      break;

    case TOKEN_SYS_CALL:
      // Handle system calls with multiple parameters.
      process_syscall_parameters(current, fp);
      break;

    default:
      // If an unknown AST node type is encountered, print an error message.
      fprintf(stderr, "Unknown AST node type: %d\n", current->type);
      break;
    }

    // Move to the next statement in the block.
    current = current->next;
  }
}

/**
 * @brief Collects all string literals from the AST and adds them to the data
 * section.
 *
 * This function iterates through the AST and collects all string literals.
 * These string literals are then added to the data section of the assembly
 * output. This process ensures that any string literals used in the program are
 * defined in the data section with unique labels.
 *
 * @param ast              The AST representing the program.
 * @param fp               The output file pointer where the data section will
 * be written.
 */
void collect_strings(ASTNode *ast, FILE *fp) {
  fprintf(fp, "section .data\n");

  // Reset string table for the current compilation process.
  for (int i = 0; i < string_table_count; i++) {
    free(string_table[i].value); // Free previous string values.
    free(string_table[i].label); // Free previous string labels.
  }
  string_table_count = 0; // Reset string count.
  string_counter = 0;     // Reset the string counter.
  current_context_id = 0; // Reset the context ID for string tracking.

  ASTNode *current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      ASTNode *block = current->left;
      if (block && block->type == TOKEN_LBRACE) {
        ASTNode *stmt = block->left;
        while (stmt != NULL) {
          // Handle MOVE statements with string literals.
          if (stmt->type == TOKEN_MOVE && stmt->right &&
              stmt->right->type == TOKEN_STRING) {
            char *str_value = stmt->right->value;
            size_t len = strlen(str_value);

            // Remove quotes from the string if present.
            if (len >= 2 && str_value[0] == '"' && str_value[len - 1] == '"') {
              // Generate a label for this string and add it to the data
              // section.
              char label[32];
              sprintf(label, "str_%d", string_counter++);
              fprintf(fp, "    %s db %s, 0\n", label, str_value);

              // Add the string to the string table.
              add_string_to_table(str_value, label);

              // Update the AST node to reference the label instead of the
              // string literal.
              free(stmt->right->value);
              stmt->right->value = strdup(label);
              stmt->right->type = TOKEN_LABEL; // Change the type to LABEL.
            }
          }

          // Handle SYS_CALL statements with string literals.
          else if (stmt->type == TOKEN_SYS_CALL) {
            start_new_string_context(); // Start a new context for string
                                        // operations in this syscall.

            ASTNode *param = stmt->left;
            while (param) {
              if (param->type == TOKEN_STRING) {
                char *str_value = param->value;
                size_t len = strlen(str_value);

                // Remove quotes from the string if present.
                if (len >= 2 && str_value[0] == '"' &&
                    str_value[len - 1] == '"') {
                  // Generate a label for the string and add it to the data
                  // section.
                  char label[32];
                  sprintf(label, "str_%d", string_counter++);
                  fprintf(fp, "    %s db %s, 0\n", label, str_value);

                  // Add the string to the string table.
                  add_string_to_table(str_value, label);

                  // Update the AST node to reference the label instead of the
                  // string literal.
                  free(param->value);
                  param->value = strdup(label);
                  param->type = TOKEN_LABEL; // Change the type to LABEL.
                }
              }
              param = param->next;
            }
          }
          stmt = stmt->next; // Move to the next statement in the block.
        }
      }
    }
    current = current->next; // Move to the next node in the AST.
  }

  fprintf(fp, "\n");
}

/**
 * @brief Generates the NASM code for the program.
 *
 * This function processes the AST and generates the corresponding NASM assembly
 * code. It first collects all string literals in the data section, then
 * processes each function and block of statements, generating the appropriate
 * assembly instructions. The result is written to the specified output file.
 *
 * @param ast              The AST representing the program.
 * @param output_file      The path to the output file where the NASM code will
 * be written.
 */
void generate_nasm(ASTNode *ast, const char *output_file) {
  FILE *fp = fopen(output_file, "w");

  if (!fp) {
    fprintf(stderr, "Failed to open output file: %s\n", output_file);
    return;
  }

  // First pass: collect all string literals into the data section.
  collect_strings(ast, fp);

  // Start the code section.
  fprintf(fp, "section .text\n");
  fprintf(fp, "global _start\n\n");

  // Add standard exit code at the beginning.
  fprintf(fp, "_exit:\n");
  fprintf(fp, "    mov eax, 1      ; exit system call\n");
  fprintf(fp, "    xor ebx, ebx    ; exit code 0\n");
  fprintf(fp, "    int 0x80        ; call kernel\n\n");

  // Process labels and code blocks.
  ASTNode *current = ast;
  bool found_main = false;

  // First, look for the main function.
  while (current != NULL) {
    if (current->type == TOKEN_LABEL && strcmp(current->value, "main") == 0) {
      found_main = true;
      break;
    }
    current = current->next;
  }

  // If the main function is found, add a jump to it from the _start label.
  if (found_main) {
    fprintf(fp, "_start:\n");
    fprintf(fp, "    jmp main     ; Call the main function\n\n");
  }

  // Now, process all functions in the AST.
  current = ast;
  while (current != NULL) {
    if (current->type == TOKEN_LABEL) {
      // Write function name and code block.
      fprintf(fp, "%s:\n", current->value);

      if (current->left && current->left->type == TOKEN_LBRACE) {
        char *data_strings[100]; // Assuming a maximum of 100 strings per block.
        generate_block(fp, current->left, data_strings, current->value);
      }

      // Add exit code at the end of the main function.
      if (strcmp(current->value, "main") == 0) {
        fprintf(fp, "    jmp _exit\n");
      } else {
        fprintf(fp, "    ret\n");
      }

      fprintf(fp, "\n");
    }

    current = current->next; // Move to the next node in the AST.
  }

  // If no main function is found, add a default _start label to exit
  // immediately.
  if (!found_main) {
    fprintf(fp, "_start:\n");
    fprintf(fp, "    ; No main function found, exiting directly\n");
    fprintf(fp, "    jmp _exit\n");
  }

  // Cleanup the string table after code generation.
  for (int i = 0; i < string_table_count; i++) {
    free(string_table[i].value);
    free(string_table[i].label);
  }
  string_table_count = 0;

  fclose(fp);

  printf("NASM code successfully generated: %s\n", output_file);
}
