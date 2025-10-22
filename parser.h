// parser.h
#pragma once
#include "hyperCNL.h"
#include "tokens.h"
#include "string.h"
#include "birchutils.h"

typedef String* (*fucntion)(Token*);

#define empty(x)    (!(x)->fun)

struct s_stack{
    Token token;
    fucntion fun;
    struct s_stack * next;
    struct s_stack * prev;
};

typedef struct s_stack Stack;

// Constructors

Stack * mkstack(void);

Stack * push(Garbage *, Stack *, Token);
Stack * apop(Garbage *, Stack *, TokenType);
Stack * findlast(Stack *);
Stack * scopy(Stack *);