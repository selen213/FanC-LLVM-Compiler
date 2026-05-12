# FanC Compiler – LLVM IR Code Generation

A compiler for the **FanC programming language** built using **Flex, Bison, and LLVM IR**.

This project implements lexical analysis, parsing, semantic handling, and intermediate code generation for a C-like programming language, translating FanC programs into executable LLVM IR.

---

## Overview

The compiler supports a complete compilation pipeline including:

- Lexical Analysis
- Syntax Parsing
- Semantic Processing
- LLVM IR Code Generation
- Runtime Error Handling

The generated LLVM IR code can be executed directly using LLVM’s `lli` interpreter.

This project focuses on low-level systems concepts, compiler design, intermediate representations, and control-flow construction.

---

# Features

## Language Support
- `int`, `byte`, `bool`, and `string` types
- Variable declarations and assignments
- Arithmetic operations
- Relational and logical expressions
- Function declarations and calls
- Return statements

## Control Flow
- `if / else`
- `while`
- `break`
- `continue`

## Compiler Functionality
- LLVM IR generation
- Stack allocation using `alloca`
- SSA-style temporary register generation
- Short-circuit boolean evaluation
- Runtime division-by-zero detection
- Local variable management
- Function call generation
- Label and branch generation
- Control Flow Graph (CFG) construction

---

# Technologies Used

- C++
- Flex
- Bison
- LLVM IR
- Makefile
- Linux / Unix Environment

---

# Example

## FanC Source Code

```c
int main() {
    int x = 5;
    int y = 2;

    if (x > y) {
        printi(x);
    }

    return 0;
}
```

## Generated LLVM IR

```llvm
%t1 = icmp sgt i32 %x, %y
br i1 %t1, label %if_true, label %if_false
```

---

# Key Concepts Implemented

- Intermediate Representation (IR)
- Static Single Assignment (SSA)
- Stack-based memory allocation
- Control Flow Graphs (CFG)
- Short-circuit evaluation
- Runtime semantic validation
- Visitor-based code generation
- LLVM instruction generation
- Label and branching systems

---

# Runtime Error Handling

The compiler includes runtime detection for division-by-zero operations and generates LLVM IR that prints:

```text
Error division by zero
```

before terminating execution.

---

# What I Learned

Through this project I gained hands-on experience with:

- Compiler architecture
- Parsing theory
- LLVM IR generation
- Systems programming concepts
- Memory management
- Control-flow implementation
- Runtime semantics
- Low-level code generation
- Backend-oriented software engineering

---

# Future Improvements

- Optimization passes
- Register allocation
- Better type system support
- Arrays and structures
- Function overloading
- LLVM optimization integration

---

# Author

Developed by Selen Abu Shakra as part of a Compiler Construction project.
