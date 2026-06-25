// parser.c

#include "parser.h"

String * id(String * s, Token * t){ 
    return s;
}

function findfun(Token _){
    return (function)0;
}

Stack * findlast(Stack * s){
    Stack * p, * last;

    if(!s){
        return (Stack*)0;
    }
    for(p=s; p->next; p=p->next){
        last = p;
    }
    
    return (p) ? p: last;
}

Stack * mkentry(){
    int16 size;
    Stack * p;

    size = sizeof(struct s_stack);
    p = (Stack*)malloc($i size);
    assert(p);
    zero($1 p, size);

    return p;
}

Stack * mkstack(int16 size){
    int16 n;
    Stack * p, * last, * first;
    assert(size);

    first=last = (Stack *)0;

    for(n=size; n; n--){
        p = mkentry();
        if(!first){
            first = p;
        }
        p->prev = last;
        if(last){
            last->next = p;
        }
        last = p;
    }
    first->length = size;

    return first;
}

Stack * stack_index(Stack * s, signed short int idx){  // cause we want index -1(last elem) and so far
    signed short int n;
    Stack * p;

    assert(s);
    assert(idx < s->length && idx >= -s->length);

    if(s->length == 1){
        return s;
    }

    if(idx < 0){
        for(n = -1, p = findlast(s); n > idx; n--, p = p->prev);
    }
    else{
        for(n = 0, p = s; n < idx; n++, p = p->next);
    }

    return p;
}

void printstack(Stack * s){  // only for debugging and stuff
    Stack * p;
    int16 n;

    printf("\nSize of stack: %d\n", s->length);
    for(n=0, p = s; n < s->length; n++, p = p->next){
        printf(".token = '%s'\n", p->token.contents.start->value);
        printf(".fun = %p\n", (void*)p->fun);
        printf(".next = %p\n", (void*)p->next);
        printf(".prev = %p\n", (void*)p->prev);
        printf("\n");
    }
    printf("\n");

    return;
}

Stack * stcopy(Garbage * g, Stack * old){
    int16 n;
    Stack * np, * op, * new;

    assert(old && old->length);

    new = mkstack(old->length);

    if(!new){
        return (Stack*)0;
    }

    for(n = 0, np = new, op = old; n < old->length; n++, np = np->next, op = op->next){
        copyentry(np, op);
        if(g){
            addgc(g, op);
        }
    }

    return new;
}

Stack * push(Garbage * g, Stack * s, Token t){
    Stack * s_, * entry, * last;
    
    assert(s && t.type && s->length);

    s_ = stcopy(g, s);
    if(!s_){
        return (Stack *)0;
    }

    entry = mkentry();
    last = findlast(s_);
    if(!last){
        return (Stack*)0;
    }

    last->next = entry;
    entry->prev = last;
    s_->length++;
    memorycopy(&entry->token, &t, sizeof(struct s_token));
    entry->fun = findfun(t);

    return s_;
}

STuple * apop(Garbage * g, Stack * s, Tag type){
    STuple * ret;
    int16 size;
    Stack * p, * s_, * last;
    int16 x;

    assert(s && s->length && type);

    size = sizeof(struct s_stuple);
    ret = (STuple*)malloc($i size);
    assert(ret);

    s_ = stcopy(g, s);
    if(!s_){
        goto error;
    }

    if(s_->length == 1){
        if(s_->token.contents.start->type != type){
            goto error;
        }

        ret->xs = (Stack *)0;
        memorycopy($1 &ret->x,$1 &s_->token, sizeof(struct s_token));
        if(g){
            addgc(g, s_);
        }
        goto end;
    }

    for(x = 0, p = findlast(s_); x < s_->length; x++, last = p, p = p->prev){
        if(p->token.contents.start->type == type){
            break;
        }
    }

    
    if(p->token.contents.start->type != type){
        goto error;
    }

    memorycopy($1 &ret->x, $1 &p->token, sizeof(struct s_token));

    if(!p->next){
        p->prev->next = (Stack*)0;
        s_->length--;
        if(g){
            addgc(g, p);
        }
        ret->xs = s_;
    }
    else if(!p->prev){
        p = p->next;
        ret->xs = p;
        p->prev = (Stack *)0;
        p->length = s_->length;
        p->length--;
        if(g){
            addgc(g, s_);
        }
    }
    else{
        p->prev->next = p->next;
        p->next->prev = p->prev;
        s_->length--;
        ret->xs = s_;
    }
    goto end;

    error:
        ret->xs = (Stack *)0;
        ret->x.contents.start = 0;
    end:
    

    return ret;
}


/* ================================================================== */
/*  Recursive-descent parser with AST construction                    */
/* ================================================================== */

/*
 * Internal helpers
 */

/* Return a pointer to the current token, or NULL if stream is empty. */
static Token *ps_peek(ParseState *ps)
{
    if (!ps || ps->pos >= ps->length) return (Token *)0;
    return &ps->tokens[ps->pos];
}

/* Consume the current token and advance the cursor. */
static Token *ps_consume(ParseState *ps)
{
    Token *t;
    if (!ps || ps->pos >= ps->length) return (Token *)0;
    t = &ps->tokens[ps->pos];
    ps->pos++;
    return t;
}

/* Return the tag value from any token variant – available for future
 * passes (e.g. code generation) that need a uniform value accessor. */
__attribute__((unused))
static int8 *token_value(Token *t)
{
    if (!t) return (int8 *)0;
    switch (t->type) {
        case tagstart:  return t->contents.start->value;
        case tagend:    return t->contents.end->value;
        case selfclosed:return t->contents.self->value;
        case text:      return t->contents.texttoken->value;
        default:        return (int8 *)0;
    }
}

/* ------------------------------------------------------------------ */

/*
 * is_whitespace_only()
 * Returns true if every character in s is a space, tab, CR, or LF.
 * Used to silently skip indentation/formatting text tokens that appear
 * between tags in multi-line source files.
 */
static bool is_whitespace_only(const int8 *s)
{
    if (!s) return true;
    for (; *s; s++) {
        if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n')
            return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
ASTNode *parse_text(Garbage *g, ParseState *ps)
{
    Token   *t;
    ASTNode *node;

    t = ps_peek(ps);
    if (!t || t->type != text) {
        fprintf(stderr, "parse_text(): expected text token\n");
        return (ASTNode *)0;
    }

    ps_consume(ps);
    node = ast_create_text(g, t->contents.texttoken->value);
    return node;
}

/* ------------------------------------------------------------------ */

/*
 * parse_element()
 *
 * Handles two forms:
 *   1. Self-closed:  <br />  or  <var name="x" value="10" />
 *   2. Paired:       <b> {children} </b>  or  <if condition="x > 5"> … </if>
 *
 * After constructing the ASTNode, any attributes stored in the token
 * are transferred to the node via ast_add_attribute().
 *
 * Syntax errors are reported to stderr and NULL is returned.
 */
ASTNode *parse_element(Garbage *g, ParseState *ps)
{
    Token   *open, *t;
    ASTNode *node, *child;
    int8    *open_tag;
    int16    i;

    t = ps_peek(ps);
    if (!t) {
        fprintf(stderr, "parse_element(): unexpected end of token stream\n");
        return (ASTNode *)0;
    }

    /* ---- self-closed tag ---- */
    if (t->type == selfclosed) {
        ps_consume(ps);
        node = ast_create_element(g, t->contents.self->value);
        /* attach attributes */
        for (i = 0; i < t->contents.self->attr_count; i++) {
            ast_add_attribute(g, node,
                              t->contents.self->attrs[i].name,
                              t->contents.self->attrs[i].value);
        }
        return node;
    }

    /* ---- opening tag ---- */
    if (t->type != tagstart) {
        fprintf(stderr, "parse_element(): expected opening tag, got token type %d\n",
                (int)t->type);
        return (ASTNode *)0;
    }

    open     = ps_consume(ps);
    open_tag = open->contents.start->value;
    node     = ast_create_element(g, open_tag);

    /* attach attributes from the opening tag */
    for (i = 0; i < open->contents.start->attr_count; i++) {
        ast_add_attribute(g, node,
                          open->contents.start->attrs[i].name,
                          open->contents.start->attrs[i].value);
    }

    /* ---- children loop ---- */
    for (;;) {
        t = ps_peek(ps);
        if (!t) {
            fprintf(stderr, "parse_element(): unclosed tag <%s>\n", (char *)open_tag);
            return (ASTNode *)0;
        }

        if (t->type == tagend) {
            if (!stringcompare(t->contents.end->value, open_tag)) {
                fprintf(stderr,
                        "parse_element(): mismatched tags: opened <%s> closed </%s>\n",
                        (char *)open_tag,
                        (char *)t->contents.end->value);
                return (ASTNode *)0;
            }
            ps_consume(ps);
            break;
        }

        if (t->type == text) {
            /* skip whitespace-only text (indentation between tags) */
            if (is_whitespace_only(t->contents.texttoken->value)) {
                ps_consume(ps);
                continue;
            }
            child = parse_text(g, ps);
        } else if (t->type == tagstart || t->type == selfclosed) {
            child = parse_element(g, ps);
        } else {
            fprintf(stderr,
                    "parse_element(): unexpected token type %d inside <%s>\n",
                    (int)t->type, (char *)open_tag);
            return (ASTNode *)0;
        }

        if (!child) return (ASTNode *)0;
        ast_add_child(node, child);
    }

    return node;
}

/* ------------------------------------------------------------------ */

/*
 * parse_document()
 *
 * Top-level entry point.
 *
 * Tokenises `src`, then parses the token stream as:
 *   document ::= element
 *
 * (A real document is a single root element – <html> … </html> – per
 * the project's grammar.bnf.)
 *
 * Returns an AST_PROGRAM node whose sole child is the root element,
 * or NULL on error.
 */
ASTNode *parse_document(Garbage *g, String *src)
{
    Tokens     *xs;
    ParseState  ps;
    ASTNode    *program, *root_elem;
    Token      *t;

    if (!g || !src || !src->length) {
        fprintf(stderr, "parse_document(): null or empty input\n");
        return (ASTNode *)0;
    }

    /* --- Lex --- */
    xs = lexer(src);
    if (!xs || !xs->length) {
        fprintf(stderr, "parse_document(): lexer produced no tokens\n");
        return (ASTNode *)0;
    }

    /* --- Set up cursor --- */
    ps.tokens = xs->ts;
    ps.length = xs->length;
    ps.pos    = 0;

    /* --- Create the PROGRAM root node --- */
    program = ast_create_element(g, (const int8 *)"PROGRAM");
    program->type = AST_PROGRAM;

    /* --- Parse top-level element(s) --- */
    while (ps.pos < ps.length) {
        t = ps_peek(&ps);
        if (!t) break;

        if (t->type == tagstart || t->type == selfclosed) {
            root_elem = parse_element(g, &ps);
            if (!root_elem) {
                return (ASTNode *)0;
            }
            ast_add_child(program, root_elem);
        } else if (t->type == text) {
            /* skip whitespace-only formatting text at the top level */
            if (is_whitespace_only(t->contents.texttoken->value)) {
                ps_consume(&ps);
                continue;
            }
            ASTNode *txt = parse_text(g, &ps);
            if (!txt) return (ASTNode *)0;
            ast_add_child(program, txt);
        } else {
            fprintf(stderr,
                    "parse_document(): unexpected token type %d at top level\n",
                    (int)t->type);
            return (ASTNode *)0;
        }
    }

    /* --- Validate: exactly one root element is expected --- */
    if (!program->firstChild) {
        fprintf(stderr, "parse_document(): document is empty\n");
        return (ASTNode *)0;
    }

    return program;
}
