# Mini C++ Static Analyzer

A compiler-design project written in C++ that simulates core phases of a compiler frontend and performs static analysis on C++ source code — detecting lexical, syntactic, semantic, and basic runtime issues before execution.

Built as part of a Compiler Design course project.

---

## Features

| Phase | What it catches |
|---|---|
| **Lexical Analysis** | Invalid characters, unterminated string/char literals |
| **Syntax Analysis** | Mismatched brackets `{}` `()` `[]`, missing semicolons |
| **Semantic Analysis** | Undeclared variables, redeclarations, type mismatches |
| **Runtime Checks** | Division by zero, array index out-of-bounds |
| **Warnings** | Potentially unsafe array accesses |

---

## Build & Run

**Requirements:** Any C++11-compatible compiler (g++, clang++)

```bash
g++ -std=c++11 -o analyzer main.cpp
./analyzer
```

Enter C++ code line by line. Type `END` on a new line to finish input and trigger analysis.

---

## Sample Session

**Input:**
```
int main() {
    int x;
    x = "hello";
    int arr[3];
    arr[5] = 1;
    int y = z + 1;
    float a = 10 / 0;
END
```

**Output:**
```
==== Semantic Errors ====
Line 3: Type mismatch: cannot assign 'string' to 'int'
  >> x = "hello";
     ^
Line 6: Use of undeclared variable 'z'
  >> int y = z + 1;
     ^

==== Runtime Errors ====
Line 7: Division by zero
  >> float a = 10 / 0;
               ^

==== Warnings ====
Line 5: Array index out-of-bounds for 'arr'
  >> arr[5] = 1;
     ^

==== Summary ====
Total Errors: 3
Total Warnings: 1
```

---

## How It Works

```
Input Code
    │
    ▼
┌─────────┐     Tokens (keyword, identifier,
│  Lexer  │ ──► number, operator, separator,
└─────────┘     string/char literal, invalid)
    │
    ▼
┌────────────────────────┐
│  Parser + Semantic     │  ──► Symbol Table (type, scope,
│  Analyzer              │       initialized, size)
└────────────────────────┘
    │
    ▼
┌─────────────────┐
│  Error Reporter │ ──► Categorized issues with line/column
└─────────────────┘
```

---

## Project Structure

```
mini-cpp-static-analyzer/
├── main.cpp       # Lexer, parser, semantic analyzer, error reporter
└── README.md
```

---

## Known Limitations

- Single-file input only (no multi-file or `#include` expansion)
- No function scope isolation — all variables share one symbol table
- Type inference is literal-only; expressions like `x + y` are not fully type-checked
- Control flow analysis (unreachable code, uninitialized use paths) not implemented
- Semicolon detection is line-based, not token-based — may produce false positives on multi-line expressions

---

## Technologies

- C++11
- STL: `stack`, `map`, `vector`, `set`

---

## Author

**Asliraf** — IIUC, CSE  
Compiler Lab Course
