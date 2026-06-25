/* lexer.h */
#pragma once
#include "hyperCNL.h"
#include "tokens.h"
#include "string.h"
#include "birchutils.h"

/* Lexer sub-stages */
enum e_stage{
    none       = 0,
    newtoken   = 1,
    readtoken  = 2,   /* reading tag name or text body                  */
    readattr   = 3,   /* reading an attribute name (after tag-name gap)  */
    readval    = 4    /* reading an attribute value (inside double-quotes)*/
};
typedef enum e_stage Stage;

/* Maximum number of attributes per tag */
#define MAX_ATTRS   16

struct s_state{
    Stage     stage;
    TokenType type;
    int8      buf[256];               /* tag name or text accumulator   */
    int8     *cur;
    /* Attribute accumulation – populated while inside a tag header */
    AttrPair  attrs[MAX_ATTRS];       /* scratch array for current tag  */
    int16     attr_count;
    int8      attr_name[ATTR_NAME_MAX];
    int8     *attr_name_cur;
    int8      attr_val [ATTR_VALUE_MAX];
    int8     *attr_val_cur;
    bool      in_value;               /* true once past the '=' quote   */
};
typedef struct s_state State;

/* Constructors */
State * mkstate(void);

Tokens * lexer(String *);
Tokens * lexer_(Garbage *, String *, Tokens *, State *);

/* Diagnostic */
void dump_tokens(Token *tokens, int16 length);
