/* lexer.c */
#include "lexer.h"

State * mkstate(){
    State * s;
    int16 size;

    size = sizeof(struct s_state);
    s = (State*)malloc($i size);
    assert(s);
    zero($1 s, size);
    zero(s->buf, 256);
    s->cur = $1 0;
    s->type = 0;
    s->stage = none;

    return s;

}

Tokens * lexer_(Garbage * g, String * s, Tokens * xs, State * state){
    Token * t;
    Tuple tuple;
    int8 c, cc;
    String * s_;

    tuple = get(s);
    c = tuple.c;
    s_ = tuple.s;
    TokenType type;

    if(!c && !s_){
        // We're done
    }
    switch(state->stage){
        case none:
            break;

        case newtoken:
            if(c == '<'){
                cc = peek(s_);    
                if(cc == '/'){
                    type = tagend;
                }
                else{
                    type = tagstart;
                }
            }
            else{
                type = text;
            }
            state->type = type;
            zero(state->buf, 256);
            state->cur = state->buf;
            state->stage = readtoken;
            return lexer_(g, s_, xs, state);
            break;

        case readtoken:
            if(c == '/'){
            return lexer_(g, s_, xs, state);
            }
            break;

        default:
            return (Tokens *)0;
            break;
    }
}

Tokens * lexer(String * s){
    Garbage * g;
    Tokens * xs;
    State * state;

    assert(s);

    if(!s->length){
        return (Tokens *)0;
    }

    g = mkgarbage();
    xs = mktokens((Garbage *)0);

    if(!g){
        if(xs){
            free(xs);
            return (Tokens *)0;
        }
    }
    if(!xs){
        if(g){
            free(g);
            return (Tokens *)0;
        }
    }
    state = mkstate();
    state->stage = newtoken;

    return lexer_(g, s, xs, state);

}

