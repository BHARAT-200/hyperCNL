/* hyperCNL.c */
#include "hyperCNL.h"

/* utilities start */
void copy(int8 *dst, int8 *src, int16 size) {
    int8 *d, *s;
    int16 n;

    for(n=size, d=dst, s=src; n; n--, d++, s++){
        *d = *s;
    }

    return;
}

int16 nstoh(int16 srcport) {
    int16 dstport;  
    int8 a, b;

    a = ((srcport & 0xff00) >> 8);
    b = (srcport & 0xff);
    dstport = 0;
    dstport = (b << 8) + a;

    return dstport;
}

void zero(int8 *str, int16 size) {
    int8 *p;
    int16 n;

    for (n=0, p=str; n<size; n++, p++){
        *p = 0;
    }

    return;
}

void printhex(int8 *str, int16 size, int8 delim) {
    int8 *p;
    int16 n;

    for (p=str, n=size; n; n--, p++) {
        printf("%.02x", *p);
        if (delim)
            printf("%c", delim);
        fflush(stdout);
    }
    printf("\n");

    return;
}

int16 stringlen(int8 *str){
    int16 n;
    int8 *p;
    assert(str);  // throws error if condition is false
    for(p=str, n=0; *p; p++, n++);
    return n;
}

void stringcopy(int8 * dst, int8 * src, int16 size){
    int16 n;
    int8 *d, *s;

    assert(src && dst && size);
    for(d=dst, s=src, n=size; n; d++, s++, n--){
        *d = *s;
    }
    
    return;
}

String * scopy(String * s){
    String *p;
    int16 size;

    assert(s && s->length);

    size = sizeof(String) + s->length;
    p = (String*) malloc($i size);
    assert(p);

    zero($1 p, size);
    p->length = s->length;
    stringcopy(p->data, s->cur, s->length);
    p->cur = p->data;
    return p;
}

void memorycopy(void * dest, void * src, int16 size){
    int8 * d = $1 dest;
    const int8 * s = (const int8*) src;

    for(int16 i = 0; i < size; i++, d++, s++){
        *d = *s;
    }

    return;
}

/* utilities end */

/* Tokens start */


Token * mktagstart(Garbage * g, int8 * value){
    int16 size, msize;
    Tagstart * p;
    Token * ret; 

    size = stringlen(value);
    msize = sizeof(struct s_tagstart) + size;
    p = (Tagstart*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    size = sizeof(struct s_token);
    ret = (Token*)malloc($i size);
    zero($1 ret, size);

    ret->type = tagstart;
    ret->contents.start = p;
    addgc(g, ret);

    return ret;
}

Token * mktagend(Garbage * g, int8 * value){
    int16 size, msize;
    Tagend * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_tagend) + size;
    p = (Tagend*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    size = sizeof(struct s_token);
    ret = (Token*)malloc($i size);
    zero($1 ret, size);

    ret->type = tagend;
    ret->contents.end = p;
    addgc(g, ret);

    return ret;
}

Token * mkselfclosed(Garbage * g, int8 * value){
    int16 size, msize;
    Selfclosed * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_selfclosed) + size;
    p = (Selfclosed*)malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    size = sizeof(struct s_token);
    ret = (Token*)malloc($i size);
    zero($1 ret, size);
    
    ret->type = selfclosed;
    ret->contents.self = p;
    addgc(g, ret);
    return ret;
}

Token * mktext(Garbage * g, int8 * value){
    int16 size, msize;
    Text * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_texttoken) + size;
    p = (Text*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    size = sizeof(struct s_token);
    ret = (Token*)malloc($i size);
    zero($1 ret, size);
    
    ret->type = text;
    ret->contents.texttoken = p;
    addgc(g, ret);
    return ret;
}

Token * mktoken(Garbage * g, TokenType type, int8 * value){
    void * ptr;
    Token * ret;
    ret = (Token*)0;

    switch(type){
        case text:        ret = mktext(g, value);          break;
        case tagstart:    ret = mktagstart(g, value);      break;
        case tagend:      ret = mktagend(g, value);        break;
        case selfclosed:  ret = mkselfclosed(g, value);    break;
        default: 
            fprintf(stderr, "mktoken(): bad input\n");
            exit(-1);

            break;
    }
    if(!ret)
        return(Token*)0;

    switch (type) {
        case text:       ptr = ret->contents.texttoken; break;
        case tagstart:   ptr = ret->contents.start;     break;
        case tagend:     ptr = ret->contents.end;       break;
        case selfclosed: ptr = ret->contents.self;      break;
    }
    addgc(g, ptr);
    return ret;
}

int8 * showtoken(Garbage *g, Token token){
    int8 * ret;
    int8 * tmp =(int8*) malloc(256);
    addgc(g, tmp);

    assert(token.type);
    ret = tmp;
    zero(ret, 256);
    switch(token.type){
        case text:
            snprintf($c tmp, 255, "%s", token.contents.texttoken->value);
            break;
        case tagstart:
            snprintf($c tmp, 255, "<%s>", token.contents.start->value);
            break;
        case tagend:
            snprintf($c tmp, 255, "</%s>", token.contents.end->value);
            break;
        case selfclosed:
            snprintf($c tmp, 255, "<%s />", token.contents.self->value);
            break;
        default: 
            break;
    }

    return ret;
}

int8 * showtokens(Garbage * g, Tokens tokens){
    int8 * p, * cur;
    static int8 buf[20480];
    int16 total, n, i;
    Token * t;
    zero(buf, sizeof(buf));
    total = 0;
    cur = buf;

    for(i=tokens.length, t=tokens.ts; i; i--, t++){
        p = showtoken(g, *t);
        if(!p){
            break;
        }
        if(!(*p)){
            continue;
        }

        n = stringlen(p);

        total += n;
        if(total >= sizeof(buf)){
            break;
        }
        stringcopy(cur, p, n);
        cur += n;
    }
    return buf;
}

Tokens * tcopy(Garbage * g, Tokens * old){
    Tokens * new;
    Token * t;
    int16 size;

    assert(old && old->length);

    size = sizeof(struct s_tokens);
    new = (Tokens*) malloc($i size);
    assert(new);

    zero($1 new, size);
    new->length = old->length;
    size = (new->length) * sizeof(struct s_token);
    t = (Token*)malloc($i size);
    assert(t);
    zero($1 t, size);

    memorycopy(t, old->ts, size);
    new->ts = t;
    addgc(g, old->ts);
    addgc(g, old);

    return new;
}

TTuple tget(Garbage * g, Tokens * old){
    Tokens * new;
    Token x;

    assert(g && old);
    if(!old->length){
        goto fail;
    }

    x = *(mktoken(g, old->ts->type ,old->ts->contents.texttoken->value));

    new = tcopy(g, old);
    if(!new){
        goto fail;
    }

    new->ts++;
    new->length--;

    TTuple ret = {
        .xs = new,
        .x = x
    };
    return ret;
    
    fail:
        TTuple err = {0};
        return err;
}

/* Tokens end */

Tuple get(String * s){
    String * new;
    int8 c;
    assert(s);
    if(!s->length){
        Tuple err = {0};
        return err;
    }

    c = *s->cur;

    new = scopy(s);
    if(!new){
        Tuple err = {0};
        return err;
    }

    new->cur++;
    new->length--;

    Tuple ret = {
        .s = new,
        .c = c
    };
    sdestroy(s);
    
    return ret;
}

String *mkstring(int8 *str){
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

/* A Small Garbage Collector start */

Garbage * mkgarbage(){
    Garbage * p;
    int16 size;
    
    size = sizeof(struct s_garbage) * GCblocksize;
    p = (Garbage*) malloc($i size);
    assert(p);
    zero($1 p, size);

    *p->p = (void **)0;
    p->capacity = GCblocksize;
    p->size = 0;

    return p;

}

Garbage * addgc(Garbage * g, void * ptr){
    int16 size;
    assert(g && ptr);

    if(g->size >= g->capacity){
        size = sizeof(struct s_garbage) * (g->capacity + GCblocksize);
        g = (Garbage *)realloc(g, $i size);
        assert(g);
        g->capacity += GCblocksize;
    }

    g->p[g->size] = ptr;
    g->size++;

    return g;
}

Garbage * gc(Garbage * g){
    int16 n;
    Garbage *p;

    for(n = g->size-1; n; n--){
        free(g->p[n]);
    }
    free(g);

    p = mkgarbage();
    return p;
}

/* A Small Garbage Collector end */


int main(int argc, char *argv[]) {
    int16 size;
    Token * t;
    Token * t1, * t2, * t3, * t4, * t5, * t6;
    Garbage * garb;
    Tokens * old, * ts;
    TTuple tt;

    garb = mkgarbage();
    t1 = mktoken(garb, tagstart, $1 "html");
    t2 = mktoken(garb, tagstart, $1 "body");
    t3 = mktoken(garb, text, $1 "hyperCNL");
    t4 = mktoken(garb, selfclosed, $1 "br");
    t5 = mktoken(garb, tagend, $1 "body");
    t6 = mktoken(garb, tagend, $1 "html");

    size = 6 * sizeof(Token);

    t = (Token*)malloc(size);
    assert(t);
    zero($1 t, size);

    t[0] = *t1;
    t[1] = *t2;
    t[2] = *t3;
    t[3] = *t4;
    t[4] = *t5;
    t[5] = *t6;

    old =(Tokens*) malloc(sizeof(struct s_tokens));
    zero($1 old, sizeof(struct s_tokens));
    old->length = 6;
    old->ts = t;

    // Tokens * new =(Tokens*) malloc(sizeof(struct s_tokens));
    // zero($1 new, sizeof(struct s_tokens));
    // new = tcopy(garb, old);
    // printf("\nnew after tcopy, tcopy test:  %s\n", showtokens(garb, *new));

    printf("\nBefore tget: %s\n", showtokens(garb, *old));
    tt = tget(garb, old);
    printf("\nRemoved: %s\n", showtoken(garb, tt.x));
    printf("%s", showtokens(garb, *tt.xs));
    gc(garb);
}