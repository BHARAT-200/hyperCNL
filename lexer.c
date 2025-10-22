/* lexer.c */
#include "lexer.h"

State * mkstate(){
    State * s;
    int16 size = sizeof(struct s_state);

    s = (State*)malloc($i size);
    assert(s);

    zero($1 s, size);
    zero(s->buf, 256);
    s->cur = s->buf;
    s->type = 0;
    s->stage = none;

    return s;
}

Tokens * lexer(String * s){
    Garbage * g;
    Tokens * xs, * xs_;
    State * state;

    assert(s && s->length);

    g = mkgarbage();
    xs = mktokens(NULL);

    state = mkstate();
    state->stage = newtoken;
    addgc(g, state);

    xs_ = lexer_(g, s, xs, state);

    return xs_;
}

Tokens * lexer_(Garbage * g, String * s, Tokens * xs, State * state){
    Token * t = NULL;
    Tuple tuple;
    int8 c = 0, cc = 0;
    String * s_ = NULL;
    TokenType type;
    Tokens * xs_ = xs;

    if(!s || !s->length){
        return xs;
    }

    tuple = get(s);
    c = tuple.c;
    s_ = tuple.s;

    if(!c && !s_){  // base case
        return xs;
    }

    switch(state->stage){

        case none:
            state->stage = newtoken;
            return lexer_(g, s_, xs_, state);

        case newtoken:   // figure out the type of token 
            if(c == '<'){
                cc = (s_ && s_->length) ? peek(s_) : 0;

                if(cc == '/')   // tagend -> </html>
                    type = tagend;
                else   // tagstart -> <html>
                    type = tagstart;
            } 
            else {
                type = text;
            }

            state->type = type;
            zero(state->buf, 256);
            state->cur = state->buf;
            state->stage = readtoken;

            if(type == text){
                *state->cur++ = c;   // add the first char of text
            }

            return lexer_(g, s_, xs_, state);

        case readtoken:

            if(state->type == text){   // Text token 
                cc = (s_ && s_->length) ? peek(s_) : 0;

                if((!s_ || !s_->length) || (cc == '<')){
                    *state->cur++ = c;   // finish text token (append current char then produce token)

                    t = mktoken(g, text, state->buf);   // make the token
                    xs_ = tcons(g, *t, xs_);

                    zero(state->buf, 256);
                    state->cur = state->buf;
                    state->stage = newtoken;

                    return lexer_(g, s_, xs_, state);
                }

                // otherwise keep consuming text
                *state->cur++ = c;
                return lexer_(g, s_, xs_, state);
            }

            //  Non-text tag handling
            if(c == '/'){
                return lexer_(g, s_, xs_, state);
            }

            if((c == ' ') && (state->type != text)){
                cc = (s_ && s_->length) ? peek(s_) : 0;

                if(cc == '/'){
                    state->type = selfclosed;
                    return lexer_(g, s_, xs_, state);
                }   // selfclosed -> <br />

            }

            if(c == '>'){
                if(state->type == tagstart || state->type == tagend || state->type == selfclosed){
                    t = mktoken(g, state->type, state->buf);   // make the token
                    xs_ = tcons(g, *t, xs_);   // add the token to list of tokens
                    
                    zero(state->buf, 256);
                    state->cur = state->buf;
                    state->stage = newtoken;

                    return lexer_(g, s_, xs_, state);
                }
            }

            // overflow protection
            if((state->cur - state->buf) >= 254)
                return xs_;

            *state->cur++ = c;
            return lexer_(g, s_, xs_, state);

        default:
            return xs_;
    }

    return xs_;
}