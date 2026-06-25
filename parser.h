// parser.h
#pragma once
#include "hyperCNL.h"
#include "tokens.h"
#include "string.h"
#include "birchutils.h"
#include "ast.h"

typedef String* (*function)(String *, Token *);

#define empty(x)    (!(x)->fun)
#define copyentry(dst, src)     do{                                             \
    memorycopy($1 &(dst->token), $1 &(src->token), sizeof(struct s_token));     \
    (dst)->fun = (src)->fun;                                                    \
}while(false)
#define first(x)    (!(x).prev)


struct s_stack{
    Token token;
    function fun;
    int16 length;
    struct s_stack * next;
    struct s_stack * prev;
};
typedef struct s_stack Stack;

struct s_stuple{
    Stack * xs;
    Token x;
};
typedef struct s_stuple STuple;

// Constructors

Stack * mkstack(int16);
Stack * mkentry();

void printstack(Stack*);
Stack* stack_index(Stack*, signed short int);
Stack * push(Garbage *, Stack *, Token);
STuple * apop(Garbage *, Stack *, Tag);
Stack * findlast(Stack *);
Stack * stcopy(Garbage *, Stack *);
String * id(String *, Token *);
function findfun(Token);

/* ------------------------------------------------------------------ */
/*  Parse state – threaded through every recursive parse call          */
/* ------------------------------------------------------------------ */
/*
 * ParseState wraps the token stream as a simple cursor so the
 * recursive-descent functions can consume tokens one at a time without
 * destructively modifying the original Tokens array.
 *
 *   tokens  – pointer to the flat Token array produced by the lexer
 *   length  – number of tokens remaining
 *   pos     – index of the *next* token to consume
 */
struct s_parsestate {
    Token  *tokens;   /* base pointer into xs->ts  */
    int16   length;   /* total number of tokens    */
    int16   pos;      /* current read position     */
};
typedef struct s_parsestate ParseState;

/* ------------------------------------------------------------------ */
/*  Public parse API                                                    */
/* ------------------------------------------------------------------ */

/*
 * Top-level entry point.
 * Tokenises `src`, validates the full document, and returns the root
 * ASTNode* (type AST_PROGRAM).  All nodes are registered with `g`.
 * Returns NULL on any syntax error (error already printed to stderr).
 */
ASTNode *parse_document(Garbage *g, String *src);

/*
 * Parse a single element (opening tag, optional children, closing tag
 * – or a self-closed tag).  Advances ps->pos past the consumed tokens.
 * Returns NULL on syntax error.
 */
ASTNode *parse_element(Garbage *g, ParseState *ps);

/*
 * Parse a text token at the current position.
 * Advances ps->pos by 1.  Returns NULL if the current token is not text.
 */
ASTNode *parse_text(Garbage *g, ParseState *ps);
