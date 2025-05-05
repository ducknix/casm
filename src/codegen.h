#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include <stdio.h>

/**
 * @brief Translates the source register names to the target architecture's
 * register names.
 *
 * This function is responsible for mapping registers from the source language
 * to the appropriate registers used in the target architecture.
 *
 * @param reg The source register name (e.g., "eax", "rbx").
 * @return const char* The corresponding target register name.
 */
const char *translate_register(const char *reg);

/**
 * @brief Processes string literals and generates appropriate labels.
 *
 * This function processes a given string literal, ensuring that it is properly
 * formatted for inclusion in the code. It then generates a unique label for
 * the string literal and writes it to the data section of the generated output.
 *
 * @param string_value The string literal to be processed.
 * @param fp The file pointer for the output file to which the label will be
 * written.
 * @return char* The label associated with the string literal.
 */
char *process_string_literal(const char *string_value, FILE *fp);

/**
 * @brief Generates NASM code from the provided abstract syntax tree (AST).
 *
 * This function is the main entry point for generating NASM assembly code from
 * the abstract syntax tree (AST). It translates the high-level language
 * constructs into target architecture-specific assembly instructions.
 *
 * @param ast The abstract syntax tree representing the source code.
 * @param output_file The name of the output file where the NASM code will be
 * written.
 */
void generate_nasm(ASTNode *ast, const char *output_file);

/**
 * @brief Generates assembly code for a given block of code.
 *
 * This function generates assembly code for a specific block of code,
 * translating the abstract syntax tree node into corresponding NASM
 * instructions. It also handles string literals and ensures the correct data
 * section layout for the generated assembly.
 *
 * @param fp The file pointer for the output file.
 * @param node The AST node representing the block of code.
 * @param data_section_strings A list of string literals to be included in the
 * data section.
 * @param func_name The name of the function to which the block of code belongs.
 */
void generate_block(FILE *fp, ASTNode *node, char **data_section_strings,
                    const char *func_name);

/**
 * @brief Collects all string literals in the AST and writes them to the data
 * section.
 *
 * This function traverses the AST, collects all string literals, and generates
 * labels for each string literal in the data section of the generated assembly
 * code.
 *
 * @param ast The abstract syntax tree to be traversed for string literals.
 * @param fp The file pointer for the output file where the string literals will
 * be written.
 */
void collect_strings(ASTNode *ast, FILE *fp);

#endif // CODEGEN_H
