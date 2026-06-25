/* hyperCNL.c
 * Compiler driver and runtime for hyperCNL.
 *
 * Pipeline
 * --------
 *   1. Read source from file (argv[1])
 *   2. Lex  → Tokens
 *   3. Parse → AST
 *   4. Generate → output.c
 *   5. (optional) GCC → output binary
 *
 * Usage
 * -----
 *   ./hyperCNL input.hcnl              # full compile
 *   ./hyperCNL input.hcnl --no-compile # stop after output.c
 */
#include "hyperCNL.h"
#include <string.h>     /* strcmp, strerror */

/* ================================================================== */
/*  Utilities                                                          */
/* ================================================================== */

void copy(int8 *dst, int8 *src, int16 size) {
    int8 *d, *s;
    int16 n;
    for (n=size, d=dst, s=src; n; n--, d++, s++)
        *d = *s;
}

int16 nstoh(int16 srcport) {
    int16 dstport;
    int8 a, b;
    a = ((srcport & 0xff00) >> 8);
    b = (srcport & 0xff);
    dstport = (int16)((b << 8) + a);
    return dstport;
}

void zero(int8 *str, int16 size) {
    int8 *p;
    int16 n;
    for (n=0, p=str; n<size; n++, p++)
        *p = 0;
}

void printhex(int8 *str, int16 size, int8 delim) {
    int8 *p;
    int16 n;
    for (p=str, n=size; n; n--, p++) {
        printf("%.02x", *p);
        if (delim) printf("%c", delim);
        fflush(stdout);
    }
    printf("\n");
}

int16 stringlen(int8 *str) {
    int16 n;
    int8 *p;
    assert(str);
    for (p=str, n=0; *p; p++, n++);
    return n;
}

void stringcopy(int8 *dst, int8 *src, int16 size) {
    int16 n;
    int8 *d, *s;
    assert(src && dst && size);
    for (d=dst, s=src, n=size; n; d++, s++, n--)
        *d = *s;
}

String *scopy(String *s) {
    String *p;
    int16 size;
    assert(s && s->length);
    size = sizeof(String) + s->length;
    p = (String *) malloc($i size);
    assert(p);
    zero($1 p, size);
    p->length = s->length;
    stringcopy(p->data, s->cur, s->length);
    p->cur = p->data;
    return p;
}

void memorycopy(void *dest, void *src, int16 size) {
    int8 *d = $1 dest;
    const int8 *s = (const int8 *) src;
    for (int16 i = 0; i < size; i++, d++, s++)
        *d = *s;
}

/* ================================================================== */
/*  Tokens                                                             */
/* ================================================================== */

Map tagmap[] = {
    {(int8 *) "html",    html},
    {(int8 *) "body",    body},
    {(int8 *) "b",       b},
    {(int8 *) "br",      br},
    {(int8 *) "var",     var},
    {(int8 *) "if",      iff},
    {(int8 *) "program", program},
    {(int8 *) "print",   print}
};

bool stringcompare(int8 *x, int8 *y) {
    int16 max, n;
    int8 *px, *py;
    max = min(stringlen(x), stringlen(y));
    for (n=max, px=x, py=y; n; n--, py++, px++)
        if (*px != *py) return false;
    return true;
}

Tag findtype(int8 *str) {
    int16 n;
    for (n=0; n < (int16)(sizeof(tagmap) / sizeof(tagmap[0])); n++)
        if (stringcompare(tagmap[n].str, str))
            return tagmap[n].tag;
    return (Tag)0;
}

Token *mktagstart(Garbage *g, int8 *value) {
    int16 size, msize;
    Tagstart *p;
    Token *ret;
    size  = stringlen(value);
    msize = sizeof(struct s_tagstart) + size + 1;
    p = (Tagstart *) malloc($i msize);
    assert(p);
    zero($1 p, msize);
    stringcopy(p->value, value, size);
    p->type       = findtype(p->value);
    p->attr_count = 0;
    p->attrs      = (AttrPair *)0;
    size = sizeof(struct s_token);
    ret = (Token *) malloc($i size);
    zero($1 ret, size);
    ret->type = tagstart;
    ret->contents.start = p;
    addgc(g, ret);
    return ret;
}

Token *mktagend(Garbage *g, int8 *value) {
    int16 size, msize;
    Tagend *p;
    Token *ret;
    size  = stringlen(value);
    msize = sizeof(struct s_tagend) + size + 1;
    p = (Tagend *) malloc($i msize);
    assert(p);
    zero($1 p, msize);
    stringcopy(p->value, value, size);
    p->type = findtype(p->value);
    size = sizeof(struct s_token);
    ret = (Token *) malloc($i size);
    zero($1 ret, size);
    ret->type = tagend;
    ret->contents.end = p;
    addgc(g, ret);
    return ret;
}

Token *mkselfclosed(Garbage *g, int8 *value) {
    int16 size, msize;
    Selfclosed *p;
    Token *ret;
    size  = stringlen(value);
    msize = sizeof(struct s_selfclosed) + size + 1;
    p = (Selfclosed *) malloc($i msize);
    assert(p);
    zero($1 p, msize);
    stringcopy(p->value, value, size);
    p->type       = findtype(p->value);
    p->attr_count = 0;
    p->attrs      = (AttrPair *)0;
    size = sizeof(struct s_token);
    ret = (Token *) malloc($i size);
    zero($1 ret, size);
    ret->type = selfclosed;
    ret->contents.self = p;
    addgc(g, ret);
    return ret;
}

Token *mktext(Garbage *g, int8 *value) {
    int16 size, msize;
    Text *p;
    Token *ret;
    size  = stringlen(value);
    msize = sizeof(struct s_texttoken) + size + 1;
    p = (Text *) malloc($i msize);
    assert(p);
    zero($1 p, msize);
    stringcopy(p->value, value, size);
    size = sizeof(struct s_token);
    ret = (Token *) malloc($i size);
    zero($1 ret, size);
    ret->type = text;
    ret->contents.texttoken = p;
    addgc(g, ret);
    return ret;
}

Token *mktoken(Garbage *g, TokenType type, int8 *value) {
    void  *ptr;
    Token *ret = (Token *)0;
    switch (type) {
        case text:       ret = mktext(g, value);       break;
        case tagstart:   ret = mktagstart(g, value);   break;
        case tagend:     ret = mktagend(g, value);     break;
        case selfclosed: ret = mkselfclosed(g, value); break;
        default:
            fprintf(stderr, "mktoken(): bad input\n");
            exit(-1);
    }
    if (!ret) return (Token *)0;
    switch (type) {
        case text:       ptr = ret->contents.texttoken; break;
        case tagstart:   ptr = ret->contents.start;     break;
        case tagend:     ptr = ret->contents.end;       break;
        case selfclosed: ptr = ret->contents.self;      break;
        default:         ptr = (void *)0;               break;
    }
    if (ptr) addgc(g, ptr);
    return ret;
}

Tokens *mktokens(Garbage *g) {
    int16 size;
    Tokens *p;
    size = sizeof(struct s_tokens);
    p = (Tokens *) malloc($i size);
    if (!p) return (Tokens *)0;
    zero($1 p, size);
    p->length = 0;
    p->ts = (Token *)0;
    if (g) addgc(g, p);
    return p;
}

int8 *showtoken(Garbage *g, Token token) {
    int8 *ret;
    int8 *tmp = (int8 *) malloc(256);
    addgc(g, tmp);
    assert(token.type);
    ret = tmp;
    zero(ret, 256);
    switch (token.type) {
        case text:
            snprintf($c tmp, 255, "%s", token.contents.texttoken->value);  break;
        case tagstart:
            snprintf($c tmp, 255, "<%s>", token.contents.start->value);    break;
        case tagend:
            snprintf($c tmp, 255, "</%s>", token.contents.end->value);     break;
        case selfclosed:
            snprintf($c tmp, 255, "<%s />", token.contents.self->value);   break;
        default: break;
    }
    return ret;
}

int8 *showtokens(Garbage *g, Tokens tokens) {
    int8 *p, *cur;
    static int8 buf[20480];
    int16 total, n, i;
    Token *t;
    assert(g && tokens.length);
    zero(buf, sizeof(buf));
    total = 0;
    cur = buf;
    for (i=tokens.length, t=tokens.ts; i; i--, t++) {
        p = showtoken(g, *t);
        if (!p) break;
        if (!(*p)) continue;
        n = stringlen(p);
        total += n;
        if (total >= (int16)sizeof(buf)) break;
        stringcopy(cur, p, n);
        cur += n;
    }
    return buf;
}

Tokens *tcopy(Garbage *g, Tokens *old) {
    Tokens *new;
    Token  *t;
    int16   size;
    assert(old && old->length);
    size = sizeof(struct s_tokens);
    new = (Tokens *) malloc($i size);
    assert(new);
    zero($1 new, size);
    new->length = old->length;
    size = (new->length) * sizeof(struct s_token);
    t = (Token *) malloc($i size);
    assert(t);
    zero($1 t, size);
    memorycopy(t, old->ts, size);
    new->ts = t;
    addgc(g, old->ts);
    addgc(g, old);
    return new;
}

TTuple tget(Garbage *g, Tokens *old) {
    Tokens *new;
    Token   x;
    int8   *xval;
    assert(g && old);
    if (!old->length) goto fail;
    switch (old->ts->type) {
        case tagstart:   xval = old->ts->contents.start->value;     break;
        case tagend:     xval = old->ts->contents.end->value;       break;
        case selfclosed: xval = old->ts->contents.self->value;      break;
        case text:       xval = old->ts->contents.texttoken->value; break;
        default: goto fail;
    }
    x = *(mktoken(g, old->ts->type, xval));
    new = tcopy(g, old);
    if (!new) goto fail;
    new->ts++;
    new->length--;
    TTuple ret = { .xs = new, .x = x };
    return ret;
    fail: { TTuple err = {0}; return err; }
}

Tokens *tcons(Garbage *g, Token x, Tokens *xs) {
    int16   size;
    Token  *x_, *ts;
    Tokens *xs_;
    int8   *xval;
    assert(g && x.type && xs);
    switch (x.type) {
        case tagstart:   xval = x.contents.start->value;      break;
        case tagend:     xval = x.contents.end->value;        break;
        case selfclosed: xval = x.contents.self->value;       break;
        case text:       xval = x.contents.texttoken->value;  break;
        default: return (Tokens *)0;
    }
    x_ = mktoken(g, x.type, xval);
    if (!x_) return (Tokens *)0;
    /* propagate attributes */
    if (x.type == tagstart && x.contents.start->attr_count) {
        x_->contents.start->attr_count = x.contents.start->attr_count;
        x_->contents.start->attrs      = x.contents.start->attrs;
    }
    if (x.type == selfclosed && x.contents.self->attr_count) {
        x_->contents.self->attr_count = x.contents.self->attr_count;
        x_->contents.self->attrs      = x.contents.self->attrs;
    }
    if (!xs->length) {
        xs_ = mktokens(g);
        xs_->length = 1;
        size = sizeof(struct s_token);
        ts = (Token *) malloc($i size);
        zero($1 ts, size);
        *ts = *x_;
        xs_->ts = ts;
        addgc(g, xs);
        return xs_;
    }
    xs_ = tcopy(g, xs);
    xs_->length++;
    size = sizeof(struct s_token) * xs_->length;
    ts = (Token *) realloc(xs->ts, size);
    addgc(g, xs);
    if (!ts) return (Tokens *)0;
    xs_->ts = ts;
    xs_->ts[xs->length] = *x_;
    return xs_;
}

/* ================================================================== */
/*  String / GC                                                        */
/* ================================================================== */

Tuple get(String *s) {
    String *new;
    int8 c;
    assert(s);
    if (!s->length) { Tuple err = {0}; return err; }
    c = *s->cur;
    new = scopy(s);
    if (!new) { Tuple err = {0}; return err; }
    new->cur++;
    new->length--;
    Tuple ret = { .s = new, .c = c };
    sdestroy(s);
    return ret;
}

int8 peek(String *s) {
    assert(s);
    if (!s->length) return (int8)0;
    return *s->cur;
}

String *mkstring(int8 *str) {
    String *p;
    int16 n, size;
    assert(str);
    n = stringlen(str);
    assert(n);
    size = sizeof(String) + n;
    p = (String *) malloc($i size);
    assert(p);
    zero($1 p, size);
    p->length = n;
    stringcopy(p->data, str, n);
    p->cur = p->data;
    return p;
}

Garbage *mkgarbage(void) {
    Garbage *p;
    int16 size;
    size = sizeof(struct s_garbage) * GCblocksize;
    p = (Garbage *) malloc($i size);
    assert(p);
    zero($1 p, size);
    *p->p = (void **)0;
    p->capacity = GCblocksize;
    p->size = 0;
    return p;
}

Garbage *addgc(Garbage *g, void *ptr) {
    int16 size;
    assert(g && ptr);
    if (g->size >= g->capacity) {
        size = sizeof(struct s_garbage) * (g->capacity + GCblocksize);
        g = (Garbage *) realloc(g, $i size);
        assert(g);
        g->capacity += GCblocksize;
    }
    g->p[g->size] = ptr;
    g->size++;
    return g;
}

Garbage *gc(Garbage *g) {
    int16 n;
    Garbage *p;
    for (n = g->size-1; n; n--)
        free(g->p[n]);
    free(g);
    p = mkgarbage();
    return p;
}

/* ================================================================== */
/*  Compiler driver helpers                                            */
/* ================================================================== */

/*
 * read_file()
 * Read the entire contents of `path` into a NUL-terminated heap
 * buffer.  Sets *out_len to the number of bytes read (excluding NUL).
 * Returns NULL on any error; caller must free() the buffer.
 */
static int8 *read_file(const char *path, long *out_len)
{
    FILE  *f;
    long   size;
    int8  *buf;

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s': %s\n", path, strerror(errno));
        return (int8 *)0;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: fseek failed on '%s'\n", path);
        fclose(f);
        return (int8 *)0;
    }
    size = ftell(f);
    if (size <= 0) {
        fprintf(stderr, "error: '%s' is empty or unreadable\n", path);
        fclose(f);
        return (int8 *)0;
    }
    rewind(f);

    buf = (int8 *) malloc((size_t)(size + 1));
    if (!buf) {
        fprintf(stderr, "error: out of memory reading '%s'\n", path);
        fclose(f);
        return (int8 *)0;
    }
    zero(buf, (int16)(size + 1 > 32767 ? 32767 : size + 1));

    if ((long)fread(buf, 1, (size_t)size, f) != size) {
        fprintf(stderr, "error: short read on '%s'\n", path);
        free(buf);
        fclose(f);
        return (int8 *)0;
    }
    buf[size] = 0;   /* explicit NUL termination */

    fclose(f);
    if (out_len) *out_len = size;
    return buf;
}

/*
 * run_gcc()
 * Compile `output_c` into a binary named `binary` using GCC.
 * Returns 0 on success.
 */
static int run_gcc(const char *output_c, const char *binary)
{
    char cmd[512];
    int  rc;

    snprintf(cmd, sizeof(cmd), "gcc \"%s\" -o \"%s\"", output_c, binary);
    printf("Compiling...  (%s)\n", cmd);
    rc = system(cmd);
    if (rc != 0)
        fprintf(stderr, "error: gcc exited with status %d\n", rc);
    return rc;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "hyperCNL compiler\n"
        "\n"
        "Usage:\n"
        "  %s <input.hcnl>              compile to C and invoke gcc\n"
        "  %s <input.hcnl> --no-compile stop after writing output.c\n"
        "\n"
        "Output files:\n"
        "  output.c   generated C source\n"
        "  output     compiled binary (unless --no-compile)\n",
        argv0, argv0);
}

/* ================================================================== */
/*  main                                                               */
/* ================================================================== */

int main(int argc, char *argv[])
{
    const char *input_path;
    const char *output_c   = "output.c";
    const char *binary     = "output";
    bool        no_compile = false;

    int8    *src_buf = (int8 *)0;
    long     src_len  = 0;
    String  *src_str  = (String *)0;
    Garbage *g        = (Garbage *)0;
    ASTNode *tree     = (ASTNode *)0;
    FILE    *outf     = (FILE *)0;
    int      rc       = 0;

    /* ---- argument parsing ---- */
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    input_path = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--no-compile") == 0) {
            no_compile = true;
        } else if (strcmp(argv[i], "--dump") == 0) {
            /* handled below after lexing */
        } else {
            fprintf(stderr, "error: unknown option '%s'\n\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* detect --dump flag */
    bool dump_mode = false;
    for (int i = 2; i < argc; i++)
        if (strcmp(argv[i], "--dump") == 0) dump_mode = true;

    /* ----------------------------------------------------------------
     * Stage 1 – read source file
     * ---------------------------------------------------------------- */
    printf("Reading '%s'...\n", input_path);
    src_buf = read_file(input_path, &src_len);
    if (!src_buf) return 1;
    printf("  %ld byte(s) read\n", src_len);

    /* ----------------------------------------------------------------
     * Stage 2 – lex and parse
     * ---------------------------------------------------------------- */
    printf("Parsing...\n");
    g       = mkgarbage();
    src_str = mkstring(src_buf);
    free(src_buf);

    /* --dump: lex only, print token stream, then exit */
    if (dump_mode) {
        Tokens *dbg_xs = lexer(src_str);
        if (!dbg_xs) { fprintf(stderr, "lexer returned NULL\n"); return 1; }
        dump_tokens(dbg_xs->ts, dbg_xs->length);
        gc(g);
        return 0;
    }

    tree = parse_document(g, src_str);
    if (!tree) {
        fprintf(stderr, "error: parse failed\n");
        gc(g);
        return 1;
    }
    printf("  AST built successfully\n");

    /* ----------------------------------------------------------------
     * Stage 3 – code generation
     * ---------------------------------------------------------------- */
    printf("Generating C...\n");
    outf = fopen(output_c, "w");
    if (!outf) {
        fprintf(stderr, "error: cannot open '%s' for writing: %s\n",
                output_c, strerror(errno));
        gc(g);
        return 1;
    }

    rc = generate_c(tree, outf);
    fclose(outf);
    gc(g);

    if (rc != 0) {
        fprintf(stderr, "error: code generation failed\n");
        return 1;
    }
    printf("  Written to '%s'\n", output_c);

    /* ----------------------------------------------------------------
     * Stage 4 – compile with GCC (optional)
     * ---------------------------------------------------------------- */
    if (no_compile) {
        printf("Skipping compilation (--no-compile).\n");
        printf("To compile manually:\n");
        printf("  gcc %s -o %s\n", output_c, binary);
        return 0;
    }

    rc = run_gcc(output_c, binary);
    if (rc != 0) return 1;

    printf("Success!  Run with:  ./%s\n", binary);
    return 0;
}
