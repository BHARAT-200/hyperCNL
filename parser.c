// parser.c

#include "parser.h"

String * id(String * s, Token * t){ 
    return s;
}

function findfun(Token _){
    return (function)0;
}

Stack * findlast(Stack * s){
    Stack * p;

    if(!s){
        return (Stack*)0;
    }
    for(p=s; p; p=p->next);
    
    return (p) ? p: s;
}

Stack * mkentry(){
    int16 size;
    Stack * p;

    size = sizeof(struct s_stack);
    p = (Stack*)malloc($i size);
    assert(p);
    zero($1 p, size);

    return p;
}

Stack * mkstack(int16 size){
    int16 n;
    Stack * p, * last, * first;
    assert(size);

    first=last = (Stack *)0;

    for(n=size; n; n--){
        p = mkentry();
        if(!first){
            first = p;
        }
        p->prev = last;
        if(last){
            last->next = p;
        }
        last = p;
    }
    first->length = size;

    return first;
}

Stack * index(Stack * s, signed short int idx){  // cause we want index -1(last elem) and so far
    signed short int n;
    Stack * p;

    assert(s);
    assert(s->length >= idx);

    if(s->length == 1){
        return s;
    }

    if(idx < 0){
        for(n = 0, p = findlast(s); n > idx; n--, p = p->prev);
    }
    else{
        for(n = 0, p = s; n < idx; n++, p = p->next);
    }

    return p;
}

void printstack(Stack * s){  // only for debugging and stuff
    Stack * p;
    int16 n;

    printf("\nSize of stack: %d\n", s->length);
    for(n=0, p = s; n < s->length; n++, p = p->next){
        if (p->token.type == tagstart){
            printf(".token = '%s'\n", p->token.contents.start->value);
        }

        printf(".fun = %p\n", (void*)p->fun);
        printf(".next = %p\n", (void*)p->next);
        printf(".prev = %p\n", (void*)p->prev);
    }
    printf("\n");

    return;
}

Stack * stcopy(Garbage * g, Stack * old){
    int16 n;
    Stack * np, * op, * new;

    assert(old && old->length);

    new = mkstack(old->length);

    if(!new){
        return (Stack*)0;
    }

    for(n = 0, np = new, op = old; n < old->length; n++, np = np->next, op = op->next){
        copyentry(np, op);
        if(g){
            addgc(g, op);
        }
    }

    return new;
}

Stack * push(Garbage * g, Stack * s, Token t){
    Stack * s_, * entry, * last;
    
    assert(s && t.type && s->length);

    s_ = stcopy(g, s);
    if(!s_){
        return (Stack *)0;
    }

    entry = mkentry();
    last = index(s_, -1);
    if(!last){
        return (Stack*)0;
    }

    last->next = entry;
    entry->prev = last;
    s_->length++;
    memorycopy(&entry->token, &t, sizeof(struct s_token));
    entry->fun = findfun(t);

    return s_;
}