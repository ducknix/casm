/**
 * @file opts.c
 * @brief Command-line option parsing and configuration extraction.
 */

#include "opts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "1.0.0"

/**
 * @brief Parses command-line arguments into option structures.
 *
 * Matches arguments against known options and assigns values when necessary.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param opts Array of supported options.
 * @param num_opts Number of defined options.
 */
void parse_options(int argc, char *argv[], Option *opts, int num_opts) {
  // Process all arguments starting from index 1 (skip program name)
  for (int i = 1; i < argc; i++) {
    bool found = false;

    // Check each defined option
    for (int j = 0; j < num_opts; j++) {
      // Match long or short form
      if ((opts[j].name && strcmp(argv[i], opts[j].name) == 0) ||
          (opts[j].shortName && strcmp(argv[i], opts[j].shortName) == 0)) {

        found = true;

        // If the option requires a value, grab the next argument
        if (opts[j].hasValue) {
          if (i + 1 < argc && argv[i + 1][0] != '-') {
            opts[j].value = argv[i + 1];
            i++; // Skip the value on the next iteration
          } else {
            fprintf(stderr, "Warning: %s option requires a value.\n", argv[i]);
          }
        } else {
          // For flag-style options, set a dummy value like "1"
          opts[j].value = "1";
        }
        break;
      }
    }

    // If argument was not a known option
    if (!found) {
      if (argv[i][0] != '-') {
        // Assume it's the input file if not already set
        bool assigned = false;
        for (int j = 0; j < num_opts; j++) {
          if (strcmp(opts[j].name, "--input") == 0 && opts[j].value == NULL) {
            opts[j].value = argv[i];
            assigned = true;
            break;
          }
        }

        if (!assigned) {
          fprintf(stderr, "Warning: '%s' is an unknown argument.\n", argv[i]);
        }
      } else {
        fprintf(stderr, "Warning: '%s' is an unrecognized option.\n", argv[i]);
      }
    }
  }
}

/**
 * @brief Converts parsed options into a structured program configuration.
 *
 * Applies default values and handles automatic output filename generation.
 *
 * @param opts Array of parsed options.
 * @param num_opts Number of options.
 * @param config Pointer to the program configuration struct to populate.
 */
void extract_config(Option *opts, int num_opts, ProgramConfig *config) {
  // Default values
  config->verbose = false;
  config->is32Bit = true;
  config->showHelp = false;
  config->showVersion = false;
  config->inputFile = NULL;
  config->outputFile = NULL;

  // Map options to configuration
  for (int i = 0; i < num_opts; i++) {
    if (opts[i].value) {
      if (strcmp(opts[i].name, "--input") == 0) {
        config->inputFile = opts[i].value;
      } else if (strcmp(opts[i].name, "--output") == 0) {
        config->outputFile = opts[i].value;
      } else if (strcmp(opts[i].name, "--verbose") == 0) {
        config->verbose = true;
      } else if (strcmp(opts[i].name, "--32") == 0) {
        config->is32Bit = true;
      } else if (strcmp(opts[i].name, "--64") == 0) {
        config->is32Bit = false;
      } else if (strcmp(opts[i].name, "--help") == 0) {
        config->showHelp = true;
      } else if (strcmp(opts[i].name, "--version") == 0) {
        config->showVersion = true;
      }
    }
  }

  // If output file is not specified, generate from input filename
  if (config->inputFile && !config->outputFile) {
    char *dot = strrchr(config->inputFile, '.');
    if (dot) {
      size_t base_len = dot - config->inputFile;
      char *output = (char *)malloc(base_len + 5); // ".asm" + null terminator
      if (output) {
        strncpy(output, config->inputFile, base_len);
        strcpy(output + base_len, ".asm");
        config->outputFile = output;
      }
    } else {
      char *output = (char *)malloc(strlen(config->inputFile) + 5);
      if (output) {
        strcpy(output, config->inputFile);
        strcat(output, ".asm");
        config->outputFile = output;
      }
    }
  }
}

/**
 * @brief Prints help and usage information to the terminal.
 *
 * @param program_name The name of the executable.
 * @param opts Array of defined options.
 * @param num_opts Number of options.
 */
void print_help(const char *program_name, Option *opts, int num_opts) {
  printf("Usage: %s [options] <input-file>\n\n", program_name);
  printf("Available options:\n");

  for (int i = 0; i < num_opts; i++) {
    if (opts[i].shortName) {
      printf("  %s, %-15s %s\n", opts[i].shortName, opts[i].name,
             opts[i].helpText);
    } else {
      printf("  %-20s %s\n", opts[i].name, opts[i].helpText);
    }
  }

  printf("\nExamples:\n");
  printf("  %s --input program.casm --output program.asm --verbose\n",
         program_name);
  printf("  %s program.casm -v\n", program_name);
}

/**
 * @brief Prints version information of the compiler.
 */
void print_version() {
  printf("CASM Compiler v%s\n", VERSION);
  printf("A C-like Assembly language compiler.\n");
  printf("Copyright (c) 2025\n");
}
