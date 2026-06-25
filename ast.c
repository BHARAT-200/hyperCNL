/* ast.c
 * Implementation of the hyperCNL Abstract Syntax Tree.
 *
 * Memory strategy
 * ---------------
 * Nodes:
 *   Each ASTNode is a single contiguous malloc block:
 *     [ ASTNode struct | NUL-terminated tag/text bytes ]
 *   The tag/text pointer points into the same block.
 *
 * Attributes:
 *   Each ASTAttribute is also a single contiguous block:
 *     [ ASTAttribute struct | name bytes | NUL | value bytes | NUL ]
 *   The name/value pointers point into that same block.
 *
 * Every allocation is registered with the caller's Garbage so a single
 * gc() call frees the entire tree including all attributes.
 *
 * ast_free() performs a manual recursive free for callers that do not
 * use a Garbage collector.
 */
#include "hyperCNL.h"   /* pulls in ast.h, zero, stringcopy, addgc   */

/* ================================================================== */
/*  Internal helpers                                                   */
/* ================================================================== */

/* Allocate a zeroed block, assert on OOM, optionally register with GC. */
static void *ast_alloc(Garbage *g, int16 size)
{
    void *p = malloc((int)size);
    assert(p);
    zero((int8 *)p, size);
    if (g) addgc(g, p);
    return p;
}

/* strlen for int8* strings (returns 0 for NULL). */
static int16 ast_strlen(const int8 *s)
{
    int16 n = 0;
    if (!s) return 0;
    while (*s++) n++;
    return n;
}

/* Case-sensitive string equality for int8* strings. */
static bool ast_streq(const int8 *a, const int8 *b)
{
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return (*a == *b);   /* both NUL at same position */
}

/* ================================================================== */
/*  Node constructors                                                  */
/* ================================================================== */

ASTNode *ast_create_program(Garbage *g)
{
    ASTNode *node = ast_create_element(g, (const int8 *)"PROGRAM");
    node->type    = AST_PROGRAM;
    return node;
}

ASTNode *ast_create_element(Garbage *g, const int8 *tag)
{
    int16    slen  = ast_strlen(tag);
    int16    total = (int16)(sizeof(ASTNode) + slen + 1);
    ASTNode *node  = (ASTNode *)ast_alloc(g, total);
    int8    *data  = (int8 *)node + sizeof(ASTNode);

    node->type        = AST_ELEMENT;
    node->firstChild  = (ASTNode *)0;
    node->nextSibling = (ASTNode *)0;
    node->text        = (char *)0;
    node->firstAttr   = (ASTAttribute *)0;

    if (slen) stringcopy(data, (int8 *)tag, slen);
    node->tag = (char *)data;

    return node;
}

ASTNode *ast_create_text(Garbage *g, const int8 *text)
{
    int16    slen  = ast_strlen(text);
    int16    total = (int16)(sizeof(ASTNode) + slen + 1);
    ASTNode *node  = (ASTNode *)ast_alloc(g, total);
    int8    *data  = (int8 *)node + sizeof(ASTNode);

    node->type        = AST_TEXT;
    node->firstChild  = (ASTNode *)0;
    node->nextSibling = (ASTNode *)0;
    node->tag         = (char *)0;
    node->firstAttr   = (ASTAttribute *)0;

    if (slen) stringcopy(data, (int8 *)text, slen);
    node->text = (char *)data;

    return node;
}

/* ================================================================== */
/*  Tree-building                                                      */
/* ================================================================== */

void ast_add_child(ASTNode *parent, ASTNode *child)
{
    ASTNode *p;
    if (!parent || !child) return;
    if (!parent->firstChild) { parent->firstChild = child; return; }
    for (p = parent->firstChild; p->nextSibling; p = p->nextSibling);
    p->nextSibling = child;
}

/* ================================================================== */
/*  Attribute API                                                      */
/* ================================================================== */

ASTAttribute *ast_add_attribute(Garbage *g, ASTNode *node,
                                const int8 *name, const int8 *value)
{
    int16         nlen, vlen, total;
    ASTAttribute *attr, *p;
    int8         *data;

    if (!node || node->type == AST_TEXT) return (ASTAttribute *)0;

    nlen  = ast_strlen(name);
    vlen  = ast_strlen(value);
    /* layout: [ ASTAttribute | name\0 | value\0 ] */
    total = (int16)(sizeof(ASTAttribute) + nlen + 1 + vlen + 1);

    attr = (ASTAttribute *)ast_alloc(g, total);
    data = (int8 *)attr + sizeof(ASTAttribute);

    /* copy name */
    if (nlen) stringcopy(data, (int8 *)name, nlen);
    attr->name = (char *)data;
    data += nlen + 1;           /* skip NUL (already zero from ast_alloc) */

    /* copy value */
    if (vlen) stringcopy(data, (int8 *)value, vlen);
    attr->value = (char *)data;

    attr->next = (ASTAttribute *)0;

    /* append to the attribute list */
    if (!node->firstAttr) {
        node->firstAttr = attr;
    } else {
        for (p = node->firstAttr; p->next; p = p->next);
        p->next = attr;
    }

    return attr;
}

char *ast_get_attribute(ASTNode *node, const int8 *name)
{
    ASTAttribute *a;
    if (!node || !name) return (char *)0;
    for (a = node->firstAttr; a; a = a->next) {
        if (ast_streq((const int8 *)a->name, name)) {
            return a->value;
        }
    }
    return (char *)0;
}

/* ================================================================== */
/*  Debug printer                                                      */
/* ================================================================== */

void ast_print(ASTNode *node, int depth)
{
    int           i;
    ASTAttribute *a;
    ASTNode      *child;

    if (!node) return;

    /* indentation */
    for (i = 0; i < depth; i++) printf("  ");

    switch (node->type) {
        case AST_PROGRAM:
            printf("PROGRAM\n");
            break;
        case AST_ELEMENT:
            printf("%s\n", node->tag ? node->tag : "(null)");
            /* print attributes one level deeper */
            for (a = node->firstAttr; a; a = a->next) {
                for (i = 0; i < depth + 1; i++) printf("  ");
                printf("%s=\"%s\"\n",
                       a->name  ? a->name  : "",
                       a->value ? a->value : "");
            }
            break;
        case AST_TEXT:
            printf("\"%s\"\n", node->text ? node->text : "");
            break;
        default:
            printf("(unknown node)\n");
            break;
    }

    /* recurse into children */
    for (child = node->firstChild; child; child = child->nextSibling) {
        ast_print(child, depth + 1);
    }
}

/* ================================================================== */
/*  Manual free (no Garbage collector)                                 */
/* ================================================================== */

void ast_free(ASTNode *node)
{
    ASTNode      *child, *next_child;
    ASTAttribute *attr,  *next_attr;

    if (!node) return;

    /* free children */
    child = node->firstChild;
    while (child) {
        next_child = child->nextSibling;
        ast_free(child);
        child = next_child;
    }

    /* free attributes */
    attr = node->firstAttr;
    while (attr) {
        next_attr = attr->next;
        free(attr);
        attr = next_attr;
    }

    free(node);
}
