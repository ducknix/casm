#ifndef OPTS_H
#define OPTS_H

#include <stdbool.h>

typedef struct {
  const char *name;      
  const char *shortName; 
  bool hasValue;         
  char *value;           
  const char *helpText;  
} Option;

typedef struct {
  char *inputFile;  
  char *outputFile; 
  bool verbose;     
  bool is32Bit;     
  bool showHelp;    
  bool showVersion; 
} ProgramConfig;

void parse_options(int argc, char *argv[], Option *opts, int num_opts);

void extract_config(Option *opts, int num_opts, ProgramConfig *config);

void print_help(const char *program_name, Option *opts, int num_opts);

void print_version(void);

#endif 
