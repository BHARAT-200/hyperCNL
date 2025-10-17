/* tokens.h */
#pragma once
#include "string.h"

enum e_tag{
    html = 1,
    body = 2,
    b = 3,
    br = 4
};
typedef enum e_tag Tag;

struct s_tagstart{
    Tag type:3;
    // Attributes 
    int8 value[];  // "html"
};
typedef struct s_tagstart Tagstart;

struct s_tagend{
    Tag type:3;
    int8 value[];
};
typedef struct s_tagend Tagend;

struct s_selfclosed{
    Tag type:3;
    int8 value[];
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