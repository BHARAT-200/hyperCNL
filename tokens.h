/* tokens.h */
#pragma once
#include "string.h"


enum e_tag{
    html    = 1,
    body    = 2,
    b       = 3,
    br      = 4,
    var     = 5,
    iff     = 6,    /* "if" is a C keyword; the tag string is still "if"      */
    program = 7,    /* <program> … </program>  — root container               */
    print   = 8     /* <print>text</print>  → printf("text\n");               */
};
typedef enum e_tag Tag;

/* ------------------------------------------------------------------ */
/*  Attribute key/value pair stored directly in the token              */
/* ------------------------------------------------------------------ */
#define ATTR_NAME_MAX   64
#define ATTR_VALUE_MAX  256

struct s_attrpair {
    int8 name [ATTR_NAME_MAX];
    int8 value[ATTR_VALUE_MAX];
};
typedef struct s_attrpair AttrPair;

/* ------------------------------------------------------------------ */
/*  Tag token structs – Tagstart and Selfclosed carry attributes       */
/* ------------------------------------------------------------------ */

struct s_tagstart{
    Tag    type:3;
    int16  attr_count;   /* number of valid entries in attrs[]        */
    AttrPair *attrs;     /* heap-allocated array, registered with GC  */
    int8   value[];      /* tag name, e.g. "html"                     */
};
typedef struct s_tagstart Tagstart;

struct s_tagend{
    Tag type:3;
    int8 value[];
};
typedef struct s_tagend Tagend;

struct s_selfclosed{
    Tag    type:3;
    int16  attr_count;
    AttrPair *attrs;
    int8   value[];
};
typedef struct s_selfclosed Selfclosed;

struct s_texttoken{
    Tag type:3;
    int8 value[];
};
typedef struct s_texttoken Text;

enum e_tokentype{
    text = 1,
    tagstart = 2,
    tagend = 3,
    selfclosed = 4
};
typedef enum e_tokentype TokenType;

struct s_token{
    TokenType type:3;
    union{
        Text *texttoken;
        Tagstart *start;
        Tagend *end;
        Selfclosed *self;
    } contents;
};
typedef struct s_token Token;

struct s_tokens{
    int16 length:16;
    Token *ts;
};
typedef struct s_tokens Tokens;

struct s_ttuple{
    Tokens * xs;
    Token x;
};
typedef struct s_ttuple TTuple;

struct s_map{
    int8 * str;
    Tag tag;
};
typedef struct s_map Map;

#define destroytoken(t)     free(t)
// \ = line continuation, used do while because macro won't end after 1st semicolon unlike in if else
#define destroytokens(ts)   do{         \
    int16 _n;                           \
    for(_n = 0; _n < (x).length; _n++){ \
        destroytoken((x).ts  + _n);     \
    }                                   \
    free((x).ts);                       \
} while(false);                         \


int8 * showtoken(Garbage *, Token);
int8 * showtokens(Garbage *, Tokens);
Tokens * tcopy(Garbage *, Tokens *);
TTuple tget(Garbage *, Tokens *);
Tokens * tcons(Garbage *, Token, Tokens *);


// Constructors
Token * mktoken(Garbage *, TokenType, int8 *);
Tokens * mktokens(Garbage *);
Token * mktagstart(Garbage *, int8 *);
Token * mktagend(Garbage *, int8 *);
Token * mkselfclosed(Garbage *, int8 *);
Token * mktext(Garbage *, int8 *);
Tag findtype(int8*);