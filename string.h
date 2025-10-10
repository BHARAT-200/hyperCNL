// string.h

typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int int64;

struct s_string{
    int16 length:16;
    int8 * cur;
    int8 data[];
};
typedef struct s_string String;

struct s_tuple{
    String *s;
    int8 c;
};
typedef struct s_tuple Tuple;

struct s_garbage{
    int16 capacity:16;
    int16 size:16;
    void *p[];
};
typedef struct s_garbage Garbage;