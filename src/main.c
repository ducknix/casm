/**
 * @file main.c
 * @brief Main source file for the CASM.
 *
 * Handles command-line argument parsing, source file reading, token generation,
 * AST creation, and NASM code generation.
 */

#include "codegen.h"
#include "lexer.h"
#include "opts.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Recursively prints the Abstract Syntax Tree (AST).
 *
 * This function is useful during development for visualizing the structure of
 * the AST.
 *
 * @param node Pointer to the AST node.
 * @param indent Current indentation level (for tree hierarchy).
 */
void print_ast(ASTNode *node, int indent) {
  if (node == NULL)
    return;

  for (int i = 0; i < indent; i++) {
    printf("  ");
  }

  printf("%s (Type: %d)\n", node->value, node->type);

  print_ast(node->left, indent + 1);
  print_ast(node->right, indent + 1);
  print_ast(node->next, indent);
}

/**
 * @brief Entry point of the program.
 *
 * Parses CLI options, reads input, performs lexical and syntax analysis,
 * and generates NASM assembly output.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int 0 on success, 1 on error.
 */
int main(int argc, char *argv[]) {
  // Define command-line options
  Option options[] = {
      {"--input", "-i", true, NULL, "Specifies the input source file"},
      {"--output", "-o", true, NULL, "Specifies the output file"},
      {"--verbose", "-v", false, NULL, "Enables verbose output"},
      {"--32", NULL, false, NULL, "Generates 32-bit code (default)"},
      {"--64", NULL, false, NULL, "Generates 64-bit code (not supported yet)"},
      {"--help", "-h", false, NULL, "Displays this help message"},
      {"--version", NULL, false, NULL, "Displays version information"}};
  int num_options = sizeof(options) / sizeof(options[0]);

  ProgramConfig config;

  // Parse command-line arguments
  parse_options(argc, argv, options, num_options);
  extract_config(options, num_options, &config);

  // Display help or version info if requested
  if (config.showHelp) {
    print_help(argv[0], options, num_options);
    return 0;
  }

  if (config.showVersion) {
    print_version();
    return 0;
  }

  // Check for input file
  if (!config.inputFile) {
    fprintf(stderr, "Error: No input file specified.\n");
    print_help(argv[0], options, num_options);
    return 1;
  }

  // Display configuration details in verbose mode
  if (config.verbose) {
    printf("Input file: %s\n", config.inputFile);
    printf("Output file: %s\n", config.outputFile);
    printf("Target architecture: %s\n", config.is32Bit ? "32-bit" : "64-bit");
  }

  // Open the input file
  FILE *file = fopen(config.inputFile, "r");
  if (!file) {
    perror("File open error");
    return 1;
  }

  // Read the entire source file
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *source_code = malloc(file_size + 1);
  if (!source_code) {
    perror("Memory allocation error");
    fclose(file);
    return 1;
  }

  fread(source_code, 1, file_size, file);
  source_code[file_size] = '\0';
  fclose(file);

  // Run lexer
  Token *tokens = lexer(source_code);
  if (!tokens) {
    fprintf(stderr, "Lexer error: Failed to generate tokens.\n");
    free(source_code);
    return 1;
  }

  // Print tokens if verbose
  if (config.verbose) {
    printf("\n=== Token List ===\n");
    Token *temp = tokens;
    while (temp) {
      printf("Token: %s (Type: %d)\n", temp->value, temp->type);
      temp = temp->next;
    }
  }

  // Run parser
  ASTNode *ast = parse_all(&tokens);
  if (!ast) {
    fprintf(stderr, "Parser error: Failed to build AST.\n");
    free(source_code);
    free_tokens(tokens);
    return 1;
  }

  // Print AST if verbose
  if (config.verbose) {
    printf("\n=== Abstract Syntax Tree ===\n");
    print_ast(ast, 0);
  }

  // Generate NASM code
  if (config.verbose) {
    printf("\n=== Generating NASM code ===\n");
  }

  generate_nasm(ast, config.outputFile);

  if (config.verbose) {
    printf("NASM code successfully generated: %s\n", config.outputFile);
  }

  // Free generated output filename if it was auto-created
  if (config.inputFile && !options[1].value) {
    free(config.outputFile);
  }

  // Cleanup
  free(source_code);
  free_tokens(tokens);
  free_ast_node(ast);

  return 0;
}
