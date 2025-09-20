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
/* utilities end */



int8 * showtokens(Tokens tokens){
    int8 * p, * cur;
    static int8 buf[20480];
    int16 total, n, i;
    Token * t;
    zero(buf, sizeof(buf));
    total = 0;
    cur = buf;

    for(i=tokens.length, t=tokens.ts; i; i--, t++){
        p = showtoken(*t);
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

int8 * showtoken(Token token){
    int8 * ret;
    static int8 tmp[256];

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


Token * mktoken(TokenType type, int8 * value){
    switch(type){
        case text:        return mktext(value);
        case tagstart:    return mktagstart(value);
        case tagend:      return mktagend(value);
        case selfclosed:  return mkselfclosed(value);
        default: 
            fprintf(stderr, "mktoken(): bad input\n");
            exit(-1);

            break;
    }

    return (Token*)0;
}

Token * mktagstart(int8 * value){
    int16 size, msize;
    Tagstart * p;
    Token * ret; 

    size = stringlen(value);
    msize = sizeof(struct s_tagend) + size;
    p = (Tagstart*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    static Token html = {
        .type = tagstart
    };
    html.contents.start = p;
    ret = &html;
    return ret;
}

Token * mktagend(int8 * value){
    int16 size, msize;
    Tagend * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_tagend) + size;
    p = (Tagend*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    static Token html = {
        .type = tagend
    };

    html.contents.end = p;
    ret = &html;
    return ret;
}

Token * mkselfclosed(int8 * value){
    int16 size, msize;
    Selfclosed * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_selfclosed) + size;
    p = (Selfclosed*)malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    static Token html = {
        .type = selfclosed
    };

    html.contents.self = p;
    ret = &html;
    return ret;
}

Token * mktext(int8 * value){
    int16 size, msize;
    Text * p;
    Token * ret;

    size = stringlen(value);
    msize = sizeof(struct s_texttoken) + size;
    p = (Text*) malloc($i msize);
    assert(p);

    zero($1 p, msize);
    stringcopy(p->value, value, size);

    static Token html = {
        .type = text
    };
    html.contents.texttoken = p;
    ret = &html;
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

int main(int argc, char *argv[]) {
    // Tuple t; String *S;
    // S = mkstring((int8 *) "Hello");
    // t = get(S);
    // printf("c = %c, new = %s\n", t.c, t.s->cur);

    int16 size;
    Token * t;
    Token * t1, * t2, * t3, * t4, * t5, * t6;

    t1 = mktoken(tagstart, $1 "html");
    t2 = mktoken(tagstart, $1 "body");
    t3 = mktoken(text, $1 "hyperCNL");
    t4 = mktoken(selfclosed, $1 "br");
    t5 = mktoken(tagend, $1 "body");
    t6 = mktoken(tagend, $1 "html");

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

    Tokens ts = {
        .length = 6,
        .ts = t
    };

    printf("%s", showtokens(ts));
    // destroytokens(ts);
    free(ts.ts);
}