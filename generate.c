/* generate.c
 * C code generation phase for hyperCNL.
 *
 * Design notes
 * ------------
 * The generator is a pure recursive tree-walk.  It holds no state
 * beyond the current FILE* and indent level, making it trivially
 * re-entrant and easy to extend.
 *
 * Tag dispatch is done with a strcmp-style helper (tag_is) rather
 * than the internal enum so that generate.c stays decoupled from the
 * tokens layer – it only depends on ast.h (through generate.h) and
 * the C standard library.
 *
 * Indentation
 * -----------
 * emit_indent(out, depth) writes (depth * 4) spaces.  Depth starts
 * at 0 for the body of main(); each nested block adds 1.
 *
 * Tag mapping summary
 * -------------------
 *  program   → #include preamble + int main(){ … return 0; }
 *  var       → int <name> = <value>;
 *  print     → printf("<text>\n");
 *  if        → if(<condition>){ … }
 *  (unknown) → comment + stderr warning
 */
#include "generate.h"
#include <string.h>     /* strcmp */

/* ================================================================== */
/*  Internal helpers                                                   */
/* ================================================================== */

/* Compare a node's tag string against a literal. */
static int tag_is(ASTNode *node, const char *name)
{
    if (!node || !node->tag) return 0;
    return strcmp(node->tag, name) == 0;
}

/* Write (indent * 4) spaces to out. */
static void emit_indent(FILE *out, int indent)
{
    int i;
    for (i = 0; i < indent * 4; i++) {
        fputc(' ', out);
    }
}

/*
 * get_text_content()
 * Return the text content of the first AST_TEXT child of `node`, or
 * NULL if there is none.  Used by <print> to extract the message.
 */
static const char *get_text_content(ASTNode *node)
{
    ASTNode *child;
    if (!node) return NULL;
    for (child = node->firstChild; child; child = child->nextSibling) {
        if (child->type == AST_TEXT && child->text) {
            return child->text;
        }
    }
    return NULL;
}

/* ================================================================== */
/*  Per-tag emitters                                                   */
/* ================================================================== */

/* <var name="x" value="10" />  →  int x = 10; */
static void emit_var(ASTNode *node, FILE *out, int indent)
{
    char *name  = ast_get_attribute(node, (const unsigned char *)"name");
    char *value = ast_get_attribute(node, (const unsigned char *)"value");

    if (!name || !value) {
        fprintf(stderr,
                "generate: <var> missing '%s' attribute – skipped\n",
                !name ? "name" : "value");
        return;
    }

    emit_indent(out, indent);
    fprintf(out, "int %s = %s;\n", name, value);
}

/* <print>Hello</print>  →  printf("Hello\n"); */
static void emit_print(ASTNode *node, FILE *out, int indent)
{
    const char *text = get_text_content(node);

    if (!text) {
        fprintf(stderr,
                "generate: <print> has no text content – emitting empty printf\n");
        text = "";
    }

    emit_indent(out, indent);
    fprintf(out, "printf(\"%s\\n\");\n", text);
}

/*
 * <if condition="x > 5"> … </if>
 *
 *   if(x > 5){
 *       …children…
 *   }
 */
static void emit_if(ASTNode *node, FILE *out, int indent)
{
    char    *cond = ast_get_attribute(node, (const unsigned char *)"condition");
    ASTNode *child;

    if (!cond) {
        fprintf(stderr,
                "generate: <if> missing 'condition' attribute – skipped\n");
        return;
    }

    emit_indent(out, indent);
    fprintf(out, "if(%s){\n", cond);

    for (child = node->firstChild; child; child = child->nextSibling) {
        emit_node(child, out, indent + 1);
    }

    emit_indent(out, indent);
    fprintf(out, "}\n");
}

/* ================================================================== */
/*  emit_node – central dispatch                                       */
/* ================================================================== */

void emit_node(ASTNode *node, FILE *out, int indent)
{
    ASTNode *child;

    if (!node) return;

    /* ---- text nodes ---- */
    if (node->type == AST_TEXT) {
        /*
         * Bare text inside <program> or other unknown containers is
         * emitted as a C line comment so the output stays valid.
         */
        emit_indent(out, indent);
        fprintf(out, "/* %s */\n", node->text ? node->text : "");
        return;
    }

    /* ---- element nodes ---- */
    if (node->type != AST_ELEMENT && node->type != AST_PROGRAM) {
        return;     /* guard – should never happen */
    }

    /* program -------------------------------------------------------- */
    if (node->type == AST_PROGRAM || tag_is(node, "PROGRAM")) {
        /*
         * The PROGRAM node itself is handled by generate_c().
         * If somehow emit_node is called on it directly, recurse into
         * its children at the same indent level.
         */
        for (child = node->firstChild; child; child = child->nextSibling) {
            emit_node(child, out, indent);
        }
        return;
    }

    /* <program> tag (distinct from AST_PROGRAM root) ----------------- */
    if (tag_is(node, "program")) {
        /*
         * A <program> element nested inside the document root acts as
         * the main body container.  generate_c() emits the boilerplate
         * around it; here we just recurse into children.
         */
        for (child = node->firstChild; child; child = child->nextSibling) {
            emit_node(child, out, indent);
        }
        return;
    }

    /* <var> ---------------------------------------------------------- */
    if (tag_is(node, "var")) {
        emit_var(node, out, indent);
        return;
    }

    /* <print> -------------------------------------------------------- */
    if (tag_is(node, "print")) {
        emit_print(node, out, indent);
        return;
    }

    /* <if> ----------------------------------------------------------- */
    if (tag_is(node, "if")) {
        emit_if(node, out, indent);
        return;
    }

    /* unknown tag ---------------------------------------------------- */
    fprintf(stderr,
            "generate: unknown tag <%s> – emitting comment\n",
            node->tag ? node->tag : "?");
    emit_indent(out, indent);
    fprintf(out, "/* unsupported tag: <%s> */\n",
            node->tag ? node->tag : "?");
}

/* ================================================================== */
/*  generate_c – public entry point                                    */
/* ================================================================== */

int generate_c(ASTNode *root, FILE *out)
{
    ASTNode *child;         /* child of PROGRAM (should be <program>) */
    ASTNode *stmt;          /* statements inside <program>             */

    if (!root || !out) {
        fprintf(stderr, "generate_c(): null root or output stream\n");
        return -1;
    }

    /*
     * Expect the tree shape:
     *
     *   PROGRAM (AST_PROGRAM)
     *     └── program (AST_ELEMENT, tag="program")
     *           ├── var …
     *           ├── print …
     *           └── if …
     *
     * We iterate over the direct children of the root PROGRAM node.
     * Each child that is a <program> element becomes the body of main().
     * Other top-level children (unexpected) are silently passed through
     * emit_node so they still produce output.
     */

    /* ---- preamble ---- */
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "\n");
    fprintf(out, "int main(){\n");

    for (child = root->firstChild; child; child = child->nextSibling) {

        if (child->type == AST_ELEMENT && tag_is(child, "program")) {
            /* emit every statement inside <program> at indent=1 */
            for (stmt = child->firstChild; stmt; stmt = stmt->nextSibling) {
                emit_node(stmt, out, 1);
            }
        } else {
            /*
             * Top-level node that is not a <program> container.
             * Emit it directly so the generator is still useful even
             * when the document does not use <program>.
             */
            emit_node(child, out, 1);
        }
    }

    fprintf(out, "\n");
    fprintf(out, "    return 0;\n");
    fprintf(out, "}\n");

    return 0;
}
