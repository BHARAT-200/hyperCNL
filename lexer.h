/* lexer.h */
#pragma once
#include "hyperCNL.h"
#include "tokens.h"
#include "string.h"
#include "birchutils.h"

enum e_stage{
    none = 0,
    newtoken = 1,
    readtoken = 2,
};
typedef e_stage Stage;

struct s_state{
    Stage stage;
    TokenType type;
    int8 buf[256];
    int8 * cur;
};
typedef struct s_state State;

/* Constructors */
State * mkstate(void);

Tokens * lexer(String *);
Tokens * lexer_(Garbage *, String *, Tokens *, State *);