#pragma once
#include "lib.c"
#include "fs.h"

#define NULL 0
#define bitor |
#define or ||
#define bitand &
#define and &&

typedef uint32_t time_t;
typedef struct file FILE;

time_t time(int k){
  if(k == 0)
    return 0;
}

void* malloc(int bytes){
  return alloc(0,bytes);
}

char to_upper(char u){
  if(u >= 65 && u <= 91)
    return (char)(u + 32);
}

char to_lower(char u){
  if(u >= 97 && u <= 123)
    return (char)(u - 32);
}
//like templates in C++
#define STACK(type)  struct stack_ ## type { \
                      type* buffer;         \
                      int size;  \
                      int pos;         \
                     }

#define QUEUE(type)  struct queue_ ## type { \
                      type* buffer;         \
                      int size;  \
                      int pos;         \
                     }