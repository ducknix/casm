#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include <stdio.h>

const char *translate_register(const char *reg);

char *process_string_literal(const char *string_value, FILE *fp);

void generate_nasm(ASTNode *ast, const char *output_file);

void generate_block(FILE *fp, ASTNode *node, char **data_section_strings,
                    const char *func_name);

void collect_strings(ASTNode *ast, FILE *fp);

#endif 
