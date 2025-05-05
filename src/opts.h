#ifndef OPTS_H
#define OPTS_H

#include <stdbool.h>

/**
 * @brief Represents a command-line option.
 */
typedef struct {
  const char *name;      /**< Long option name (e.g., --output) */
  const char *shortName; /**< Short option alias (e.g., -o) */
  bool hasValue;         /**< Indicates whether the option expects a value */
  char *value;           /**< The actual value passed by the user */
  const char *helpText;  /**< Help text displayed in --help output */
} Option;

/**
 * @brief Holds the configuration extracted from command-line options.
 */
typedef struct {
  char *inputFile;  /**< Input file path */
  char *outputFile; /**< Output file path */
  bool verbose;     /**< Enables verbose output */
  bool is32Bit;     /**< Use 32-bit code generation (default: true) */
  bool showHelp;    /**< Display help message and exit */
  bool showVersion; /**< Display version info and exit */
} ProgramConfig;

/**
 * @brief Parses command-line arguments and fills the given options.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param opts Array of supported options
 * @param num_opts Number of options in the array
 */
void parse_options(int argc, char *argv[], Option *opts, int num_opts);

/**
 * @brief Converts parsed options into a usable program configuration.
 *
 * @param opts Array of parsed options
 * @param num_opts Number of options
 * @param config Pointer to a ProgramConfig structure to populate
 */
void extract_config(Option *opts, int num_opts, ProgramConfig *config);

/**
 * @brief Prints the help message with all available options.
 *
 * @param program_name The name of the executable
 * @param opts Array of defined options
 * @param num_opts Number of options
 */
void print_help(const char *program_name, Option *opts, int num_opts);

/**
 * @brief Prints the compiler's version information.
 */
void print_version(void);

#endif // OPTS_H
