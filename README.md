# hyperCNL

A small compiler that translates a tag-based markup language into C source code.

```
<program>
    <var name="x" value="10" />
    <print>Hello World</print>
    <if condition="x > 5">
        <print>Big</print>
    </if>
</program>
```

compiles to:

```c
#include <stdio.h>

int main(){
    int x = 10;
    printf("Hello World\n");
    if(x > 5){
        printf("Big\n");
    }

    return 0;
}
```

---

## Build

Requires: `cc` (clang or gcc), `make`, `gcc` (for the optional compile step).

```sh
make
```

This produces the `hypercnl` binary.

```sh
make clean    # remove all build artifacts
make run      # build, create demo.hcnl, compile and run it
make install  # install hypercnl to /usr/local/bin (requires sudo)
```

---

## Usage

```sh
./hypercnl <input.hcnl>              # compile to C, invoke gcc, produce ./output
./hypercnl <input.hcnl> --no-compile # stop after writing output.c
./hypercnl <input.hcnl> --dump       # print the raw token stream (debug)
```

### Output files

| File       | Description                          |
|------------|--------------------------------------|
| `output.c` | Generated C source                   |
| `output`   | Compiled native binary (if gcc runs) |

---

## Language reference

### `<program>`

Root container. Wraps all statements. Generates the `main()` function and the
`#include <stdio.h>` preamble.

```xml
<program>
    <!-- statements go here -->
</program>
```

### `<var>`

Declares an integer variable.

```xml
<var name="x" value="42" />
```

Generates:

```c
int x = 42;
```

### `<print>`

Prints a string to stdout followed by a newline.

```xml
<print>Hello World</print>
```

Generates:

```c
printf("Hello World\n");
```

### `<if>`

Conditional block. The `condition` attribute is emitted verbatim as the C
`if` expression, so any valid C expression works.

```xml
<if condition="x > 5">
    <print>Big</print>
</if>
```

Generates:

```c
if(x > 5){
    printf("Big\n");
}
```

---

## Example source file

Save as `hello.hcnl`:

```xml
<program>
    <var name="score" value="99" />
    <print>Score check</print>
    <if condition="score >= 90">
        <print>Grade: A</print>
    </if>
    <if condition="score < 90">
        <print>Grade: B or lower</print>
    </if>
</program>
```

Compile and run:

```sh
./hypercnl hello.hcnl
./output
```

Expected output:

```
Score check
Grade: A
```

---

## Architecture

```
input.hcnl
    │
    ▼
 Lexer (lexer.c)
    │  Produces a flat Token[] array.
    │  Handles: tagstart, tagend, selfclosed, text.
    │  Parses attribute name="value" pairs inside tags.
    ▼
 Parser (parser.c)
    │  Recursive-descent, cursor-based (ParseState).
    │  Builds an AST using first-child / next-sibling layout.
    │  Whitespace-only text tokens between tags are silently dropped.
    ▼
 AST (ast.c / ast.h)
    │  ASTNode: type, tag, text, firstChild, nextSibling, firstAttr.
    │  ASTAttribute: singly-linked name/value pairs on each element.
    │  All nodes registered with a Garbage collector; freed with gc().
    ▼
 Code generator (generate.c)
    │  Pure recursive tree-walk over the AST.
    │  Dispatches on tag name via strcmp (decoupled from token enum).
    │  Emits indented C source to a FILE*.
    ▼
 output.c  →  gcc  →  output binary
```

### Memory management

All heap allocations are registered with a `Garbage *` collector.  A single
`gc(g)` call frees everything allocated during a compilation pass.  `ast_free()`
is also available for manual cleanup without a GC instance.

---

## File listing

| File           | Role                                          |
|----------------|-----------------------------------------------|
| `tokens.h`     | Token and AttrPair struct definitions         |
| `string.h`     | String, Garbage, Tuple struct definitions     |
| `lexer.h/c`    | Tokeniser                                     |
| `parser.h/c`   | Recursive-descent parser → AST                |
| `ast.h/c`      | AST node and attribute types + API            |
| `generate.h/c` | C code generator                              |
| `hyperCNL.h/c` | Runtime utilities, GC, token constructors, main |
| `birchutils.h/c` | Low-level byte utilities (copy, zero, etc.) |
| `grammar.bnf`  | Informal BNF grammar reference                |
| `Makefile`     | Build rules                                   |
