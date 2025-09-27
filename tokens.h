/* tokens.h */
#include "string.h"

enum e_tag{
    html = 1,
    body = 2,
    b = 3,
    br = 4
};
typedef enum e_tag Tag;

struct s_tagstart{
    Tag type;
    // Attributes 
    int8 value[];  // "html"
};
typedef struct s_tagstart Tagstart;

struct s_tagend{
    Tag type;
    int8 value[];
};
typedef struct s_tagend Tagend;

struct s_selfclosed{
    Tag type;
    int8 value[];
};
typedef struct s_selfclosed Selfclosed;

struct s_texttoken{
    Tag type;
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
    TokenType type;
    union{
        Text *texttoken;
        Tagstart *start;
        Tagend *end;
        Selfclosed *self;
    } contents;
};
typedef struct s_token Token;

struct s_tokens{
    int16 length;
    Token *ts;
};
typedef struct s_tokens Tokens;

#define destroytoken(t)     free(t)
// \ = line continuation, used do while because macro won't end after 1st semicolon unlike in if else
#define destroytokens(ts)   do{         \
    int16 _n;                           \
    for(_n = 0; _n < (x).length; _n++){ \
        destroytoken((x).ts  + _n);     \
    }                                   \
    free((x).ts);                       \
} while(false);                         \
int8 * showtoken(Token);
int8 * showtokens(Tokens);


// Constructors
Token * mktoken(Garbage *, TokenType, int8 *);
Token * mktagstart(int8 * );
Token * mktagend(int8 * );
Token * mkselfclosed(int8 * );
Token * mktext(int8 * );