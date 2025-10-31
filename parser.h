// parser.h
#pragma once
#include "hyperCNL.h"
#include "tokens.h"
#include "string.h"
#include "birchutils.h"

typedef String* (*function)(String *, Token *);

#define empty(x)    (!(x)->fun)
#define copyentry(dst, src)     do{                                             \
    memorycopy($1 &(dst->token), $1 &(src->token), sizeof(struct s_token));     \
    (dst)->fun = (src)->fun;                                                    \
}while(false)
#define first(x)    (!(x).prev)

struct s_tuple{
    Stack * xs;
    Token x;
};
typedef struct s_tuple STuple;

struct s_stack{
    Token token;
    function fun;
    int16 length;
    struct s_stack * next;
    struct s_stack * prev;
};

typedef struct s_stack Stack;

// Constructors

Stack * mkstack(int16);
Stack * mkentry();

void printstack(Stack*);
Stack* index(Stack*, signed short int);
Stack * push(Garbage *, Stack *, Token);
STuple * apop(Garbage *, Stack *, TokenType);
Stack * findlast(Stack *);
Stack * stcopy(Garbage *, Stack *);
String * id(String *, Token *);
function findfun(Token);
