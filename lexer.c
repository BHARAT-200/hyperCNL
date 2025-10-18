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

Tokens * lexer(String * s){
    Garbage * g;
    Tokens * xs, * xs_;
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
    addgc(g, state);

    xs_ = lexer_(g, s, xs, state);
    gc(g);

    return xs_;
}

Tokens * lexer_(Garbage * g, String * s, Tokens * xs, State * state){
    Token * t;
    Tuple tuple;
    int8 c, cc;
    String * s_;
    TokenType type;
    Tokens * xs_;

    tuple = get(s);
    c = tuple.c;
    s_ = tuple.s;
    addgc(g, s);

    if(!c && !s_){  // base case
        return xs;
    }


    switch(state->stage){
        case none:
            break;

        case newtoken:  // figure out the type of token 
            if(c == '<'){
                cc = peek(s_);    
                if(cc == '/'){  // tagend -> </html>
                    type = tagend;
                }
                else{  // tagstart -> <html>
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


        case readtoken:  // read until this token is done
            if(c == '/'){
                return lexer_(g, s_, xs, state);
            }

            if((c == ' ')  ||  (state->type != text)) {
                cc = peek(s);

                if(cc == '/'){  // selfclosed -> <br />
                    state->type = selfclosed;
                }
                
                return lexer_(g, s_, xs_, state);
            }

            else if(c == '>'){
                if((state->type == tagstart)  ||  (state->type == tagend)  ||  (state->type == selfclosed)){

                    t = mktoken(g, state->type, state->buf);  // make the token
                    if(!t)
                        return (Tokens *)0;
                    
                    xs_ = tcons(g, *t, xs);  // add the token to list of tokens
                    if(!xs)
                        return (Tokens *)0;

                    addgc(g, xs);
                    zero(state->buf, 256);
                    state->stage = newtoken;
                    state->cur = state->buf;

                    return lexer_(g, s_, xs_, state);
                }
            }

            if(((void *)state->cur - (void *)&state->buf) >= 254)  // (no of bytes read >= 254) -> parse error (No tag can be over 254 bytes)
                return (Tokens *) 0;
            
            state->cur++;
            *state->cur = c;
            return lexer_(g, s_, xs, state);

            break;


        default:
            return (Tokens *)0;
            break;
    }
    return (Tokens *)0;
}