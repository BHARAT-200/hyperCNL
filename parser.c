// parser.c

#include "parser.h"

Stack * mkstack(){
    int16 size;
    Stack * p;

    size = sizeof(struct s_stack);
    p = (Stack*)malloc($i size);
    assert(p);
    zero($1 p, size);

    return p;
}


Stack * findlast(Stack * s){
    Stack * p;

    if(!s){
        return (Stack*)0;
    }
    for(p=s; p; p=p->next);

    return p;
}

Stack * scopy(Stack * s){
    Stack *f, * p, * new, * l;
    
    assert(s);
    if(empty(s)){
        return mkstack();
    }

    l = (Stack*)0;
    f = (Stack*)0;
    for(p=s; p; p=p->next){
        new = mkstack();
        if(!f){
            f = new;
        }
        new->fun = p->fun;
        memorycopy(&new->token, &p->token, sizeof(struct s_token));
        new->prev = l;
        if(l){
            l->next = new;
        }
        l = new;
    }

    return f;
}

Stack * push(Garbage * g, Stack * s, Token t){
    Stack * s_,  * p, * last;

    assert(s && t.type);
    if(g){
        addgc(g, s);
    }

    if(empty(s)){
        s_ = mkstack();
        last = s_;
    }
    else{
        s_ = scopy(s);
        last = findlast(s_);
    }

    // will continue tomorrow, I'm having a headache now.
}