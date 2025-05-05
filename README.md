# CASM - C-Like Assembly Compiler

CASM is a compiler that transforms a c-like assembly language into x86 NASM assembly code. It provides a more readable and programmer-friendly syntax while maintaining the power of low-level assembly programming.

## CASM vs NASM Comparison

Below is a comparison of the same "Hello World" program written in CASM and NASM:

### Hello World in CASM
```
func main {
    sys_call(4, 1, "Hello World", &strlen&);
}
```

### Hello World in NASM
```nasm
section .data
    msg db 'Hello World', 0

section .text
global _start

_start:
    mov eax, 4      ; System call number (sys_write)
    mov ebx, 1      ; File descriptor (stdout)
    mov ecx, msg    ; Message to print
    mov edx, 12     ; Message length
    int 0x80        ; Make system call
    
    mov eax, 1      ; System call number (sys_exit)
    mov ebx, 0      ; Exit code
    int 0x80        ; Make system call
```

As you can see, CASM offers a more readable syntax. Register names are more intuitive (a, b, c, d), commands are more descriptive (move, call), and the syntax is closer to C programming language structure.

## About the Project

The CASM compiler is designed to enable assembly programming with a more readable and user-friendly syntax. This compiler takes code written in a C-like syntax (CASM) and converts it into x86/x86_64 assembly code for NASM (The Netwide Assembler). The project currently targets Unix-like systems (Linux, macOS).

The compiler performs the following functions:
1. Lexical Analysis
2. Abstract Syntax Tree (AST) creation
3. NASM format code generation

## Project Structure

The project consists of the following components:

- **lexer.c/h**: Lexical analysis module that breaks CASM code into tokens
- **parser.c/h**: Module that processes tokens to create an abstract syntax tree (AST)
- **codegen.c/h**: Module that converts the AST into NASM assembly code
- **main.c**: Main entry point and module that coordinates the compiler flow

## Usage

You can use the compiler as follows:

```bash
./casm <source-file> [output-file]
```

If no output file is specified, the source file name with an `.asm` extension is used.

## CASM Syntax

The CASM language consists of the following basic structures:

### Functions

```
func function_name {
    // Commands
}
```

### Commands

```
move(destination, source);     ' Value assignment
add(destination, value);       ' Addition operation
sub(destination, value);       ' Subtraction operation
compare(value1, value2);       ' Comparison
jump(label);                   ' Unconditional jump
jump_equal(label);             ' Jump if equal
return();                      ' Return
```

## sys_call

```
sys_call(4, 1, "Hello", &strlen&);  ' sys_write with auto string length
```

### Variables

In CASM, you can use variable names like `a`, `b`, `c`, `d` to represent registers. These are converted to `eax`, `ebx`, `ecx`, `edx` registers in NASM, respectively.

### String Support

```
move(c, "Hello World");    ' Supports string literals
move(d, &strlen&);         ' Automatic string length calculation
```

This code is converted into a NASM assembly program that prints "Hello World" to standard output and exits.


## TODO List

- [x] Basic command support (move, add, sub, compare, jump, jump_equal)
- [x] Label/Function support
- [x] String literals support
- [x] System call support (call, sys_call)
- [x] Automatic string length calculation with &strlen&
- [ ] More register support (16-bit, 8-bit registers and extended register set of x86_64)
- [ ] Advanced control structures (abstractions for if-else, loops, etc.)
- [ ] Advanced syntax for memory operations
- [ ] Include system with modular code support
- [ ] Better debugging and reporting
- [ ] Optimization options
- [ ] Macro support
- [ ] Direct executable file creation feature (NASM + ld)
- [ ] Full x86_64 (64-bit) architecture support
- [ ] More comprehensive error handling and recovery mechanisms
- [ ] Support for symbolic constants and customizable data section
- [ ] Direct executable file production

## Code Generation Details

The compiler converts CASM code into an assembly file in NASM format. The generated code:

1. Places all code in the `.text` section
2. Automatically creates the program entry point (`_start`) and exit routine (`_exit`)
3. Automatically converts register names to their x86 equivalents
4. Places string literals in the `.data` section with appropriate labels

## Limitations

The current version has some limitations:

1. Only supports basic assembly commands
2. Limited number of registers (`eax`, `ebx`, `ecx`, `edx`) are supported
3. Advanced memory addressing mechanisms are not available
4. Currently supports conversion to NASM code, but does not produce executable files directly
5. Lack of optimization for 64-bit x86_64 architecture
6. String length calculation with `&strlen&` has limitations, especially when used with the `move` instruction

## Contributing

To contribute to the project:

1. Fork the repo
2. Create a new branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Create a Pull Request
