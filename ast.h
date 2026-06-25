/* ast.h
 * Abstract Syntax Tree node definitions and interface for hyperCNL.
 *
 * All AST nodes and attributes are heap-allocated and registered with
 * the caller's Garbage collector so that a single gc() call frees the
 * entire tree.
 *
 * This header includes only "string.h" (for Garbage / int8 / int16)
 * to avoid the circular dependency:
 *   hyperCNL.h -> parser.h -> ast.h -> hyperCNL.h
 *
 * ast.c itself includes hyperCNL.h so it can call zero(), stringcopy(),
 * addgc(), etc.
 */
#pragma once
#include "string.h"     /* String, Garbage, int8, int16            */
#include <stdlib.h>     /* malloc, free                             */
#include <stdio.h>      /* printf, fprintf                          */
#include <assert.h>     /* assert                                   */

/* ------------------------------------------------------------------ */
/*  Node type discriminator                                             */
/* ------------------------------------------------------------------ */
typedef enum {
    AST_PROGRAM  = 0,   /* root wrapper produced by parse_document()  */
    AST_ELEMENT  = 1,   /* an HTML/CNL element, e.g. <var>, <b>       */
    AST_TEXT     = 2    /* raw text node, e.g. "Hello"                 */
} ASTNodeType;

/* ------------------------------------------------------------------ */
/*  Attribute – singly-linked list on each element node               */
/* ------------------------------------------------------------------ */
/*
 * Each ASTAttribute is a single heap block:
 *
 *   [ ASTAttribute struct | name bytes | NUL | value bytes | NUL ]
 *
 * `name` and `value` point into that same block.
 */
typedef struct ASTAttribute {
    char               *name;
    char               *value;
    struct ASTAttribute *next;   /* next attribute on this element     */
} ASTAttribute;

/* ------------------------------------------------------------------ */
/*  Node structure                                                      */
/* ------------------------------------------------------------------ */
/*
 * Tree layout: first-child / next-sibling representation.
 *
 *   parent  (firstAttr → attr1 → attr2 → …)
 *     └── firstChild ──nextSibling──> sibling2 ──…
 *
 * `tag`       set for AST_ELEMENT and AST_PROGRAM nodes.
 * `text`      set for AST_TEXT nodes.
 * `firstAttr` head of the attribute linked-list (NULL if none).
 *
 * Tag/text string bytes are stored in the same allocation block as
 * the ASTNode struct (immediately following it in memory).
 */
typedef struct ASTNode {
    ASTNodeType      type;
    char            *tag;
    char            *text;
    struct ASTNode  *firstChild;
    struct ASTNode  *nextSibling;
    ASTAttribute    *firstAttr;
} ASTNode;

/* ------------------------------------------------------------------ */
/*  Public API – node constructors                                      */
/* ------------------------------------------------------------------ */

ASTNode *ast_create_program(Garbage *g);
ASTNode *ast_create_element(Garbage *g, const int8 *tag);
ASTNode *ast_create_text   (Garbage *g, const int8 *text);

/* ------------------------------------------------------------------ */
/*  Public API – tree building                                          */
/* ------------------------------------------------------------------ */

/* Append `child` as the last child of `parent`. */
void ast_add_child(ASTNode *parent, ASTNode *child);

/* ------------------------------------------------------------------ */
/*  Public API – attribute handling                                     */
/* ------------------------------------------------------------------ */

/*
 * ast_add_attribute()
 * Create a new ASTAttribute for (name, value), register it with `g`,
 * append it to `node`'s attribute list, and return the new attribute.
 * Returns NULL if node is NULL or not an element.
 */
ASTAttribute *ast_add_attribute(Garbage *g, ASTNode *node,
                                const int8 *name, const int8 *value);

/*
 * ast_get_attribute()
 * Search `node`'s attribute list for the first attribute whose name
 * matches `name`.  Returns a pointer to the value string, or NULL if
 * not found.  The returned pointer is valid for the lifetime of `node`.
 */
char *ast_get_attribute(ASTNode *node, const int8 *name);

/* ------------------------------------------------------------------ */
/*  Public API – debug and memory                                       */
/* ------------------------------------------------------------------ */

/*
 * ast_print()
 * Depth-first printer.  Each element's attributes are printed on
 * indented lines immediately below the element name:
 *
 *   var
 *     name="x"
 *     value="10"
 */
void ast_print(ASTNode *node, int depth);

/*
 * ast_free()
 * Recursively free the whole sub-tree rooted at `node` including all
 * ASTAttribute nodes.  Only needed when the tree was built WITHOUT a
 * Garbage collector; otherwise call gc() on the owning Garbage.
 */
void ast_free(ASTNode *node);
