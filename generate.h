/* generate.h
 * C code generation phase for hyperCNL.
 *
 * Traverses a fully-constructed AST (produced by parse_document) and
 * emits a complete, compilable C source file to a FILE stream.
 *
 * Supported tags
 * --------------
 *  <program>           Container.  Wraps emitted statements in
 *                        #include <stdio.h>
 *                        int main(){
 *                            …
 *                            return 0;
 *                        }
 *
 *  <var name="x" value="10" />
 *                      Variable declaration:
 *                        int x = 10;
 *
 *  <print>Hello</print>
 *                      Print statement:
 *                        printf("Hello\n");
 *
 *  <if condition="x > 5">
 *    <print>Big</print>
 *  </if>               Conditional block:
 *                        if(x > 5){
 *                            printf("Big\n");
 *                        }
 *
 * Usage
 * -----
 *   #include "generate.h"
 *   …
 *   generate_c(root_ast_node, stdout);          // or a FILE* to a file
 *
 * The function does not modify the AST.
 * No additional memory is allocated – the generator is purely a
 * recursive walk that writes to `out`.
 */
#pragma once
#include "ast.h"        /* ASTNode, ASTAttribute, ast_get_attribute   */
#include <stdio.h>      /* FILE                                        */

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/*
 * generate_c()
 *
 * Walk `root` (must be the AST_PROGRAM node returned by
 * parse_document) and write a complete C translation unit to `out`.
 *
 * Errors (unknown tags, missing attributes) are reported to stderr;
 * generation continues for the remaining siblings so the caller
 * receives as complete an output as possible.
 *
 * Returns 0 on success, -1 if root is NULL or not AST_PROGRAM.
 */
int generate_c(ASTNode *root, FILE *out);

/* ------------------------------------------------------------------ */
/*  Internal helper – exposed so callers can reuse for custom passes   */
/* ------------------------------------------------------------------ */

/*
 * emit_node()
 *
 * Recursively emit C code for a single AST node and all its
 * descendants.  `indent` is the current indentation level (each
 * level adds 4 spaces).
 *
 * Callers should prefer generate_c() for the common case; emit_node()
 * is useful when building further passes on top of this generator.
 */
void emit_node(ASTNode *node, FILE *out, int indent);
