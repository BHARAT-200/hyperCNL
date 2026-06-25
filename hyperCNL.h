/* hyperCNL.h */
#pragma once // used in a header file to prevent it from being included multiple times during compilation
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "tokens.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "generate.h"


#define $1 (int8 *)
#define $2 (int16)
#define $4 (int32)
#define $7 (int64)
#define $c (char *)
#define $i (int)
#define sdestroy(s)     free(s)
#define GCblocksize     1024
#define min(x,y)        ((x) < (y)) ? (x) : (y)

// constructor functions
String * mkstring(int8 *);
Garbage * mkgarbage(void);

String * scopy(String *);
Garbage * addgc(Garbage *, void *);
Garbage * gc(Garbage *);
Tuple get(String *);
int8 peek(String *);
int16 stringlen(int8 *);
void stringcopy(int8 *, int8 *, int16);
void memorycopy(void *, void *, int16);
bool stringcompare(int8*, int8*);