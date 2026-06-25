/* lexer.c */
#include "lexer.h"

/* ------------------------------------------------------------------ */
/*  mkstate                                                             */
/* ------------------------------------------------------------------ */

State * mkstate(){
    State * s;
    int16 size = sizeof(struct s_state);

    s = (State*)malloc($i size);
    assert(s);

    zero($1 s, size);
    zero(s->buf, 256);
    s->cur          = s->buf;
    s->type         = 0;
    s->stage        = none;
    s->attr_count   = 0;
    s->attr_name_cur = s->attr_name;
    s->attr_val_cur  = s->attr_val;
    s->in_value      = false;

    return s;
}

/* ------------------------------------------------------------------ */
/*  Helper: attach accumulated attrs to a Tagstart or Selfclosed token */
/* ------------------------------------------------------------------ */

static void attach_attrs(Garbage *g, Token *t, State *state)
{
    int16      size;
    AttrPair  *arr;

    if (!state->attr_count) return;

    size = (int16)(sizeof(AttrPair) * state->attr_count);
    arr  = (AttrPair*)malloc($i size);
    assert(arr);
    memorycopy(arr, state->attrs, size);
    addgc(g, arr);

    if (t->type == tagstart) {
        t->contents.start->attrs      = arr;
        t->contents.start->attr_count = state->attr_count;
    } else if (t->type == selfclosed) {
        t->contents.self->attrs      = arr;
        t->contents.self->attr_count = state->attr_count;
    }
}

/* ------------------------------------------------------------------ */
/*  Helper: reset attribute scratch buffers for a new tag              */
/* ------------------------------------------------------------------ */

static void reset_attrs(State *state)
{
    state->attr_count    = 0;
    state->in_value      = false;
    zero(state->attr_name, ATTR_NAME_MAX);
    zero(state->attr_val,  ATTR_VALUE_MAX);
    state->attr_name_cur = state->attr_name;
    state->attr_val_cur  = state->attr_val;
}

/* ------------------------------------------------------------------ */
/*  Helper: commit current attr_name/attr_val into the attrs array     */
/* ------------------------------------------------------------------ */

static void commit_attr(State *state)
{
    AttrPair *slot;
    int16     nlen, vlen;

    nlen = stringlen(state->attr_name);
    if (!nlen) return;                      /* nothing to commit */
    if (state->attr_count >= MAX_ATTRS) return;

    slot = &state->attrs[state->attr_count];
    zero($1 slot, sizeof(AttrPair));

    vlen = stringlen(state->attr_val);
    stringcopy(slot->name,  state->attr_name, nlen);
    if (vlen) stringcopy(slot->value, state->attr_val, vlen);

    state->attr_count++;

    /* Reset scratch buffers for next attribute */
    zero(state->attr_name, ATTR_NAME_MAX);
    zero(state->attr_val,  ATTR_VALUE_MAX);
    state->attr_name_cur = state->attr_name;
    state->attr_val_cur  = state->attr_val;
    state->in_value      = false;
}

/* ------------------------------------------------------------------ */
/*  Token dump helper (temporary diagnostic tool)                      */
/* ------------------------------------------------------------------ */

void dump_tokens(Token *tokens, int16 length) {
    int16 i;
    Token *t;
    printf("\n=== TOKEN DUMP (%d tokens) ===\n", (int)length);
    for (i = 0; i < length; i++) {
        t = &tokens[i];
        printf("[%2d] ", (int)i);
        switch (t->type) {
            case tagstart:
                printf("TAGSTART   <%s>  attr_count=%d\n",
                       t->contents.start->value,
                       (int)t->contents.start->attr_count);
                for (int a = 0; a < t->contents.start->attr_count; a++)
                    printf("           attr[%d] %s = \"%s\"\n", a,
                           t->contents.start->attrs[a].name,
                           t->contents.start->attrs[a].value);
                break;
            case tagend:
                printf("TAGEND     </%s>\n", t->contents.end->value);
                break;
            case selfclosed:
                printf("SELFCLOSED <%s />  attr_count=%d\n",
                       t->contents.self->value,
                       (int)t->contents.self->attr_count);
                for (int a = 0; a < t->contents.self->attr_count; a++)
                    printf("           attr[%d] %s = \"%s\"\n", a,
                           t->contents.self->attrs[a].name,
                           t->contents.self->attrs[a].value);
                break;
            case text: {
                const int8 *v = t->contents.texttoken->value;
                printf("TEXT       [");
                for (; *v; v++) {
                    switch (*v) {
                        case '\n': printf("\\n"); break;
                        case '\r': printf("\\r"); break;
                        case '\t': printf("\\t"); break;
                        case ' ':  printf(".");   break;
                        default:   printf("%c", *v);
                    }
                }
                printf("]\n");
                break;
            }
            default:
                printf("UNKNOWN    type=%d\n", (int)t->type);
        }
    }
    printf("=== END TOKEN DUMP ===\n\n");
}

/* ------------------------------------------------------------------ */
/*  lexer / lexer_                                                      */
/* ------------------------------------------------------------------ */

Tokens * lexer(String * s){
    Garbage * g;
    Tokens  * xs, * xs_;
    State   * state;

    assert(s && s->length);

    g   = mkgarbage();
    xs  = mktokens(NULL);

    state = mkstate();
    state->stage = newtoken;
    addgc(g, state);

    xs_ = lexer_(g, s, xs, state);

    return xs_;
}

Tokens * lexer_(Garbage * g, String * s, Tokens * xs, State * state){
    Token   * t = NULL;
    Tuple     tuple;
    int8      c = 0, cc = 0;
    String  * s_ = NULL;
    TokenType type;
    Tokens  * xs_ = xs;

    if(!s || !s->length){
        return xs;
    }

    tuple = get(s);
    c     = tuple.c;
    s_    = tuple.s;

    if(!c && !s_){          /* base case */
        return xs;
    }

    switch(state->stage){

    /* ---------------------------------------------------------------- */
    case none:
        state->stage = newtoken;
        return lexer_(g, s_, xs_, state);

    /* ---------------------------------------------------------------- */
    case newtoken:
        if(c == '<'){
            cc = (s_ && s_->length) ? peek(s_) : 0;
            type = (cc == '/') ? tagend : tagstart;
        } else {
            type = text;
        }

        state->type = type;
        zero(state->buf, 256);
        state->cur = state->buf;
        state->stage = readtoken;
        reset_attrs(state);

        if(type == text){
            *state->cur++ = c;          /* keep first text char */
        }

        return lexer_(g, s_, xs_, state);

    /* ---------------------------------------------------------------- */
    case readtoken:

        /* ---- text token ---- */
        if(state->type == text){
            /*
             * Stop text accumulation when we hit a '<' character.
             * At this point `c` is '<' — which belongs to the next tag,
             * not to the current text token.  Emit what we have so far
             * (without appending `c`), then re-enter newtoken processing
             * for `c` with the remaining string `s_`.
             *
             * We also stop when the input is exhausted (no s_ left).
             */
            if(c == '<'){
                /* Emit accumulated text (buf may be empty if no chars yet) */
                if(state->cur > state->buf){
                    t    = mktoken(g, text, state->buf);
                    xs_  = tcons(g, *t, xs_);
                }
                /* Re-enter newtoken with c='<' already in hand.
                 * We rebuild a one-char String for '<' and the remainder. */
                zero(state->buf, 256);
                state->cur   = state->buf;
                state->stage = newtoken;

                /* Determine tag type from the char after '<' */
                cc = (s_ && s_->length) ? peek(s_) : 0;
                TokenType ntype = (cc == '/') ? tagend : tagstart;

                state->type  = ntype;
                state->stage = readtoken;
                zero(state->buf, 256);
                state->cur   = state->buf;
                reset_attrs(state);
                /* '<' itself is not stored in buf – tag names start after it */
                return lexer_(g, s_, xs_, state);
            }

            /* Normal: check if the NEXT character is '<' (end of text) */
            cc = (s_ && s_->length) ? peek(s_) : 0;

            if((!s_ || !s_->length) || cc == '<'){
                *state->cur++ = c;      /* append last char */

                t    = mktoken(g, text, state->buf);
                xs_  = tcons(g, *t, xs_);

                zero(state->buf, 256);
                state->cur   = state->buf;
                state->stage = newtoken;

                return lexer_(g, s_, xs_, state);
            }
            *state->cur++ = c;
            return lexer_(g, s_, xs_, state);
        }

        /* ---- non-text: skip the leading '/' in </tag> ---- */
        if(c == '/'){
            return lexer_(g, s_, xs_, state);
        }

        /* ---- space inside a tag header --------------------------------
         * Once we have a tag name in buf, a space means either:
         *   a) attributes follow  → switch to readattr
         *   b) self-close prefix  → peek ahead for '/'
         * ---------------------------------------------------------------- */
        if(c == ' ' && state->type != text){
            cc = (s_ && s_->length) ? peek(s_) : 0;

            if(cc == '/'){
                /* <br /> — self-closed, no attributes on this space */
                state->type = selfclosed;
                return lexer_(g, s_, xs_, state);
            }

            /* only switch to readattr if we already have a tag name */
            if(state->cur > state->buf){
                state->stage = readattr;
                return lexer_(g, s_, xs_, state);
            }
            return lexer_(g, s_, xs_, state);
        }

        /* ---- '>' closes a tag that has no attributes ---- */
        if(c == '>'){
            if(state->type == tagstart || state->type == tagend
               || state->type == selfclosed)
            {
                t    = mktoken(g, state->type, state->buf);
                /* no attrs to attach here (none were accumulated) */
                xs_  = tcons(g, *t, xs_);

                zero(state->buf, 256);
                state->cur   = state->buf;
                state->stage = newtoken;
                reset_attrs(state);

                return lexer_(g, s_, xs_, state);
            }
        }

        /* ---- overflow guard ---- */
        if((state->cur - state->buf) >= 254)
            return xs_;

        *state->cur++ = c;
        return lexer_(g, s_, xs_, state);

    /* ---------------------------------------------------------------- */
    case readattr:
        /*
         * We are inside a tag header after the tag name, reading
         * attribute tokens of the form:  name="value"
         *
         * Characters we need to handle:
         *   '='  → start of value, next char should be '"'
         *   '"'  → opening or closing quote of value
         *   ' '  → separator between attributes (or trailing space)
         *   '/'  → beginning of self-close "/>"
         *   '>'  → end of tag
         *   else → accumulate into attr_name
         */

        if(c == '='){
            /* transition: next significant char is the opening '"' */
            state->in_value = false;    /* will flip on '"' */
            return lexer_(g, s_, xs_, state);
        }

        if(c == '"'){
            if(!state->in_value){
                /* opening quote */
                state->in_value = true;
                state->stage    = readval;
                return lexer_(g, s_, xs_, state);
            }
            /* closing quote – shouldn't normally reach here in readattr */
            return lexer_(g, s_, xs_, state);
        }

        if(c == ' '){
            /* space can separate attributes; commit any pending name */
            /* (attr without a value – rare, but safe to handle) */
            if(stringlen(state->attr_name)){
                commit_attr(state);
            }
            return lexer_(g, s_, xs_, state);
        }

        if(c == '/'){
            /* self-closing "/>" */
            state->type = selfclosed;
            return lexer_(g, s_, xs_, state);
        }

        if(c == '>'){
            /* end of tag: commit any pending bare attr, emit token */
            if(stringlen(state->attr_name)){
                commit_attr(state);
            }

            t = mktoken(g, state->type, state->buf);
            attach_attrs(g, t, state);
            xs_ = tcons(g, *t, xs_);

            zero(state->buf, 256);
            state->cur   = state->buf;
            state->stage = newtoken;
            reset_attrs(state);

            return lexer_(g, s_, xs_, state);
        }

        /* accumulate attribute name character */
        if((state->attr_name_cur - state->attr_name) < ATTR_NAME_MAX - 1){
            *state->attr_name_cur++ = c;
        }
        return lexer_(g, s_, xs_, state);

    /* ---------------------------------------------------------------- */
    case readval:
        /*
         * Inside a quoted attribute value.
         * We stay here until the closing '"'.
         */

        if(c == '"'){
            /* closing quote: commit the attribute, back to readattr */
            commit_attr(state);
            state->stage = readattr;
            return lexer_(g, s_, xs_, state);
        }

        /* accumulate value character */
        if((state->attr_val_cur - state->attr_val) < ATTR_VALUE_MAX - 1){
            *state->attr_val_cur++ = c;
        }
        return lexer_(g, s_, xs_, state);

    /* ---------------------------------------------------------------- */
    default:
        return xs_;
    }

    return xs_;
}
