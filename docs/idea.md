# Project Specification: SpeakC

## 1. Executive Summary & Vision

The goal is to design and build **SpeakC**, a programming language that compiles 1:1 into C17 (and vice versa). SpeakC is designed specifically for beginners and is structured around "programmer speak" — how developers naturally read and explain code out loud.

Unlike languages like COBOL or SQL, which are overly verbose, SpeakC preserves standard mathematical logic and structural grouping (using parentheses for function arguments) while completely eliminating noisy syntax like semicolons, curly braces `{}`, and cryptic pointer symbols.

**The transpiler itself will be written entirely in C17**, making this a self-reinforcing project: building SpeakC teaches deep C mastery, and SpeakC makes C more approachable for others.

**Library functions are not translated.** Functions like `printf`, `malloc`, `fopen`, etc. keep their original names. Only the surrounding syntax (semicolons, braces, pointer symbols, declaration style) is transformed into SpeakC's readable format.

---

## 2. Core Design Philosophies

**1:1 Mapping:** Every feature, primitive, and construct in SpeakC maps 1:1 to a standard C17 equivalent. No custom runtime, garbage collector, or standard library is built — we rely entirely on the target C17 compiler (gcc, clang, etc.).

**Bidirectional Transpilation:** The system must be capable of parsing SpeakC to standard C17, and parsing standard C17 back to SpeakC without loss of logical structure or context.

**No Semicolons or Braces:** Indentation (meaningful whitespace) defines code blocks instead of `{}`. Semicolons are eliminated entirely.

**Symbol Minimization:** Replace cryptic symbols like `*`, `&`, `->`, and complex declaration syntax with clear, conversational keywords.

**Library Transparency:** Standard library functions (`printf`, `scanf`, `malloc`, `free`, `fopen`, etc.) retain their original names. SpeakC only transforms the *syntax structure* around them, not the function names themselves.

---

## 3. Syntax Specifications & Grammar Mapping

### 3.1 Variables & Assignments

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Typed Declaration | `declare x as int` | `int x;` | Uninitialized variable |
| Initialized Declaration | `set x to 5 as int` | `int x = 5;` | Declares and assigns |
| Re-assignment | `set x to x + 5` | `x = x + 5;` | Assigns to existing variable |
| Constant | `set MAX to 100 as constant int` | `const int MAX = 100;` | Immutable value |
| Float Declaration | `set pi to 3.14 as float` | `float pi = 3.14f;` | Float literal |
| Char Declaration | `set letter to 'A' as char` | `char letter = 'A';` | Character literal |
| String (char array) | `set name to "Alice" as string` | `char name[] = "Alice";` | Syntactic sugar for char array |

### 3.2 Pointers & Memory

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Pointer Declaration | `declare p as pointer to int` | `int *p;` | Explicit pointer type |
| Address-of | `address of x` | `&x` | Evaluates to memory address |
| Pointer Init | `set p to address of x as pointer to int` | `int *p = &x;` | Declares pointer with address |
| Dereference | `value at p` | `*p` | Accesses data at address |
| Dereference Assign | `set value at p to 10` | `*p = 10;` | Writes through pointer |
| Null Pointer | `set p to nothing` | `p = NULL;` | `nothing` maps to NULL |
| Sizeof | `size of int` | `sizeof(int)` | Size of a type |
| Malloc | `set p to malloc(size of int)` | `p = malloc(sizeof(int));` | Library call, name unchanged |
| Free | `free(p)` | `free(p);` | Library call, name unchanged |

### 3.3 Arrays

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Array Declaration | `declare scores as array of 10 int` | `int scores[10];` | Fixed-size array |
| Array Init | `set nums to {1, 2, 3} as array of 3 int` | `int nums[3] = {1, 2, 3};` | Initialized array |
| Array Access | `scores[0]` | `scores[0]` | Index syntax unchanged |
| Array Assignment | `set scores[0] to 100` | `scores[0] = 100;` | Element assignment |

### 3.4 Structs

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Struct Definition | See block example below | `struct Point { int x; int y; };` | Indentation-based |
| Member Access | `x of point` | `point.x` | `of` replaces `.` |
| Pointer Member | `x of ptr` | `ptr->x` | `of` replaces `->` too |
| Struct Init | `set p to Point{5, 10}` | `struct Point p = {5, 10};` | Struct literal |

**Struct block example:**

SpeakC:
```
define Point as structure:
    x as int
    y as int
```

C17:
```c
struct Point {
    int x;
    int y;
};
```

### 3.5 Functions

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Void function | `define function greet:` | `void greet() {` | No return type = void |
| Typed function | `define function add(a as int, b as int) returns int:` | `int add(int a, int b) {` | Return type at end |
| Return | `return a + b` | `return a + b;` | Same keyword |
| Function call | `add(3, 5)` | `add(3, 5);` | Unchanged |
| Forward declaration | `declare function add(a as int, b as int) returns int` | `int add(int a, int b);` | Prototype |

**Function block example:**

SpeakC:
```
define function add(a as int, b as int) returns int:
    return a + b
```

C17:
```c
int add(int a, int b) {
    return a + b;
}
```

### 3.6 Control Flow

**If / Else:**


SpeakC:
```
if x > 5:
    printf("big\n")
else if x > 0:
    printf("small\n")
else:
    printf("zero or negative\n")
```

C17:
```c
if (x > 5) {
    printf("big\n");
} else if (x > 0) {
    printf("small\n");
} else {
    printf("zero or negative\n");
}
```

**While Loop:**

SpeakC:
```
while x > 0:
    set x to x - 1
```

C17:
```c
while (x > 0) {
    x = x - 1;
}
```

**Do-While Loop:**

SpeakC:
```
do:
    set x to x - 1
while x > 0
```

C17:
```c
do {
    x = x - 1;
} while (x > 0);
```

**For Loop:**

SpeakC:
```
for set i to 0 as int, i < 10, increment i:
    printf("%d\n", i)
```

C17:
```c
for (int i = 0; i < 10; i++) {
    printf("%d\n", i);
}
```

**Switch:**

SpeakC:
```
switch on x:
    case 1:
        printf("one\n")
        break
    case 2:
        printf("two\n")
        break
    default:
        printf("other\n")
```

C17:
```c
switch (x) {
    case 1:
        printf("one\n");
        break;
    case 2:
        printf("two\n");
        break;
    default:
        printf("other\n");
}
```

### 3.7 Compound Assignment & Increment

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Increment | `increment i` | `i++` | Post-increment |
| Decrement | `decrement i` | `i--` | Post-decrement |
| Add-assign | `add 2 to i` | `i += 2` | Compound addition |
| Subtract-assign | `subtract 2 from i` | `i -= 2` | Compound subtraction |
| Multiply-assign | `multiply i by 2` | `i *= 2` | Compound multiplication |
| Divide-assign | `divide i by 2` | `i /= 2` | Compound division |

### 3.8 Preprocessor Directives

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| System include | `include <stdio>` | `#include <stdio.h>` | `.h` appended automatically |
| Local include | `include "myfile"` | `#include "myfile.h"` | `.h` appended automatically |
| Define constant | `define MAX as 100` | `#define MAX 100` | Preprocessor constant |
| Define macro | `define SQUARE(x) as ((x)*(x))` | `#define SQUARE(x) ((x)*(x))` | Preprocessor macro |

### 3.9 Type Casting

| Concept | SpeakC Syntax | C17 Equivalent | Notes |
|---|---|---|---|
| Cast | `cast x to float` | `(float)x` | Explicit type cast |

---

## 4. Error Handling (The "Expecting" Pattern)

C doesn't have Go's multi-return error pattern, but C has its own common patterns: **NULL checks** for pointer-returning functions and **return code checks** for int-returning functions. SpeakC provides the `expecting` keyword to handle the most common case — NULL checks.

-Error handling would be done the same way, regular C does it, so feature 4 will not be implemented



## 5. Bidirectional Parser Architecture

The transpiler (written in C17) uses a 3-part pipeline:

```
                  +--------------------------------+
                  |       SpeakC Source Code        |
                  |          (.speakc)              |
                  +--------------------------------+
                             |          ^
               Translate Forward        Translate Backward
                             v          |
+---------------+     +--------------------------------+     +---------------+
|   C17 Code    | <-> |    Unified Abstract Syntax     | <-> | SpeakC Code   |
|   Emitter     |     |          Tree (AST)            |     |   Emitter     |
+---------------+     +--------------------------------+     +---------------+
        |                         ^          ^                        ^
        v                         |          |                        |
+---------------+          +----------+  +----------+                 |
|  Standard C17 |          | SpeakC   |  |   C17    |                 |
|  Source Code   | ------->| Parser   |  |  Parser  |-----------------+
|   (.c)        |          +----------+  +----------+
+---------------+
```

### Forward Translation (SpeakC → C17)

**Lexer:** Scans the SpeakC source file character-by-character, generates tokens (keywords, identifiers, literals, operators), and injects virtual `INDENT` and `DEDENT` tokens based on indentation levels.

**Parser:** A hand-written recursive descent parser that builds a custom Abstract Syntax Tree (AST). It translates the reversed English syntax (e.g., `x of point` → a MemberAccess AST node with object `point` and member `x`).

**Emitter:** Walks the AST and outputs formatting-compliant C17 code with proper braces, semicolons, and indentation, written to a standard `.c` file.

### Backward Translation (C17 → SpeakC)

**C17 Lexer:** A focused lexer for C17 primitive syntax — handles keywords (`int`, `float`, `char`, `struct`, `if`, `for`, `while`, `return`, etc.), operators, braces, and semicolons.

**C17 Parser:** A recursive descent parser for the C17 primitive subset. Since we're only handling primitive syntax (not the full C grammar with all its edge cases), this is manageable. It produces the same unified AST structure.

**SpeakC Emitter:** Walks the AST and prints SpeakC code with standardized, clean indentation — replacing braces with indentation, semicolons with newlines, and pointer syntax with keywords.

### Why No External Parser Library?

Building the C17 parser from scratch (instead of using a library like libclang) is a deliberate choice:

1. **Learning:** Writing a parser is the core educational value of this project.
2. **Scope:** We only parse C17 *primitives*, not the full C grammar. No need for a full-featured parser.
3. **Zero dependencies:** The transpiler compiles with just a C17 compiler. No external libraries needed.
4. **Control:** Full control over AST structure for clean bidirectional mapping.

---

## 6. Internal Data Structures (Built from Scratch in C)

Since C has no built-in dynamic arrays, hash maps, or string builders, these foundational data structures must be implemented as part of the project:

### Dynamic Array (Vector)
A growable array for token lists, AST child nodes, and other collections. Backed by `malloc`/`realloc` with a capacity-doubling growth strategy.

### String Builder
A buffer for constructing output strings efficiently during code emission. Avoids repeated `strcat` calls by maintaining a write cursor into a pre-allocated buffer that grows as needed.

### Arena Allocator
An arena (bump allocator) for AST nodes. Instead of individually `malloc`-ing and `free`-ing each AST node, all nodes are allocated from a large memory block. When transpilation is complete, the entire arena is freed at once. This eliminates memory leaks and simplifies cleanup.

### Token Structure
```c
typedef struct {
    TokenType type;     // TOKEN_SET, TOKEN_TO, TOKEN_AS, TOKEN_IDENT, etc.
    char *value;        // The actual text of the token
    int line;           // Source line number (for error messages)
    int column;         // Source column number
} Token;
```

### AST Node Structure
```c
typedef struct ASTNode {
    NodeType type;              // NODE_ASSIGNMENT, NODE_IF, NODE_FUNCTION, etc.
    struct ASTNode **children;  // Dynamic array of child nodes
    int child_count;
    char *value;                // Optional string value (identifier name, literal, etc.)
    DataType data_type;         // For typed nodes (int, float, char, pointer, etc.)
    int line;                   // Source line for error reporting
} ASTNode;
```

---

## 7. Developer Tooling Strategy

Instead of writing language servers and diagnostic engines from scratch, SpeakC's architecture is designed to leverage existing, world-class ecosystems.

**Syntax Highlighting:** A standard VS Code TextMate grammar (JSON) is created to token-match SpeakC's keywords (`set`, `to`, `as`, `define`, `of`, `expecting`, `declare`, `increment`, etc.).

**LSP Proxy (Autocomplete & Diagnostics):**
- The SpeakC Language Server runs as a background proxy.
- When a developer types in a `.speakc` file, the proxy generates a temporary `.c` file in memory.
- This code is sent to `clangd` (the C/C++ language server).
- The diagnostics and completions returned by `clangd` are parsed, translated back to SpeakC column offsets using a Source Map, and displayed in the IDE.

**Formatting (speakcfmt):** Re-emits any SpeakC AST cleanly, ensuring a zero-argument formatting standard for all files.

---

## 8. Potential Technical Challenges & Mitigations

### Challenge A: Chained Member Access ("of" Precedence)

When translating struct member access, standard C reads left-to-right: `game.player.position.x`.
In SpeakC, this is written as: `x of position of player of game`.

**The Issue:** A naive parser evaluating left-to-right will parse this as `(x of position) of player...`, which is logically incorrect. The `of` operator must bind right-to-left.

**The Solution:** Define the `of` keyword as a right-associative operator in the SpeakC parser grammar. This forces the parser to evaluate the expression as `x of (position of (player of game))`, which maps cleanly to C's nested member access.

### Challenge B: Indentation Tracking (Indent/Dedent Virtual Tokens)

Because SpeakC eliminates curly braces `{}` and semicolons, tracking where blocks start and end relies entirely on whitespace indentation.

**The Issue:** Mixed tabs and spaces, or completely blank lines within a block, can cause the lexer to generate erroneous syntax tokens.

**The Solution:** The Lexer must maintain an internal indentation stack (storing integers representing the indentation depth).

- When a line starts with a depth greater than the top of the stack, push the new depth and emit a virtual `INDENT` token.
- When a line starts with a depth less than the top of the stack, pop from the stack until a matching depth is found, emitting a virtual `DEDENT` token for each popped value.
- Completely blank lines or lines containing only comments should be skipped entirely by the indentation analyzer.

### Challenge C: Pointer vs. Member Access Ambiguity

In C, `point.x` (direct member) and `ptr->x` (pointer member) are different operations. In SpeakC, both use `x of point` and `x of ptr`.

**The Issue:** The transpiler must know whether a variable is a struct or a pointer-to-struct to emit the correct C operator (`.` vs `->`).

**The Solution:** Maintain a simple symbol table during parsing that tracks variable types. When the parser encounters `x of something`, it looks up `something` in the symbol table:
- If it's a struct → emit `something.x`
- If it's a pointer to struct → emit `something->x`

### Challenge D: C17 Declaration Complexity

C17 allows complex declaration forms: `int *p`, `int **pp`, `int (*fp)(int, int)`, `int arr[10]`, `const int *p`. The SpeakC parser must handle all of these in its readable keyword form.

**The Issue:** C's declaration syntax is notoriously hard to read and parse (the "clockwise/spiral rule"). SpeakC must flatten this into readable keywords while maintaining a 1:1 mapping.

**The Solution:** Use a compositional keyword system:
- `pointer to int` → `int *`
- `pointer to pointer to int` → `int **`
- `array of 10 int` → `int [10]`
- `constant pointer to int` → `const int *`
- Function pointer syntax handled as a special case: `declare fp as function pointer(int, int) returns int` → `int (*fp)(int, int)`

### Challenge E: Memory Management in the Transpiler Itself

The transpiler, being written in C, must carefully manage its own memory: token arrays, AST nodes, string buffers, symbol tables.

**The Solution:** Use an **arena allocator** pattern. Allocate all transpilation-related memory from a single arena. When transpilation is complete (or fails), free the entire arena at once. This eliminates:
- Memory leak risks from forgotten `free()` calls
- Use-after-free bugs from complex pointer chains
- The need for individual destructors on every data structure

### Challenge F: Lossless Round-Trip Translation

A major challenge in bidirectional systems is maintaining format sanity. If you transpile C17 → SpeakC → C17, the resulting C17 code should not look mutated or scrambled.

**The Issue:** C17 allows multiple declaration styles (`int x = 5;` vs `int x; x = 5;`), various brace styles (K&R vs Allman), and flexible whitespace. SpeakC needs deterministic translation rules so output remains predictable.

**The Solution:** Create strict, normalized default patterns:
- Always translate `int x = 5;` to `set x to 5 as int`
- Always emit K&R brace style in C17 output
- Auto-manage `#include` directives: when the forward transpiler finds library function calls (like `printf`), it determines the needed header and adds the appropriate `#include` to the output

---

## 9. Full Example: SpeakC Program

### SpeakC Source:

```
include <stdio>
include <stdlib>

define Point as structure:
    x as int
    y as int

define function create_point(x as int, y as int) returns pointer to Point:
    set p to malloc(size of Point) as pointer to Point
    set x of p to x
    set y of p to y
    return p

define function main returns int:
    set p to create_point(10, 20) expecting nothing:
        printf("Memory allocation failed\n")
        return 1

    printf("Point: (%d, %d)\n", x of p, y of p)
    free(p)
    return 0
```

### Transpiled C17 Output:

```c
#include <stdio.h>
#include <stdlib.h>

struct Point {
    int x;
    int y;
};

struct Point *create_point(int x, int y) {
    struct Point *p = malloc(sizeof(struct Point));
    p->x = x;
    p->y = y;
    return p;
}

int main() {
    struct Point *p = create_point(10, 20);
    if (p == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    printf("Point: (%d, %d)\n", p->x, p->y);
    free(p);
    return 0;
}
```
