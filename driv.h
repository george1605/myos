#pragma once
#include "lib.c"
#include "mem.h"
#include "drivers/blockdev.h"

int* drivbrk = DRIV_MEM + 10;

void dbgprint(char* u){
  kprint(u);
}

void dbgerr(char* u){
  perror(u);
}

struct drivlog {
  char** messages;
  int msgcnt;
  int size;
} _log;

void dbgclr(){ // clears the log
  memset(_log.messages,0,_log.msgcnt);
}

void dbglog(char* msg){
  _log.messages[++_log.msgcnt] = msg;
}

void dbgputc(struct devdrv* x){
  x->putc(x);
}

void dbgetc(struct devdrv* x){
  x->getc(x);
}

struct drivobj {
  char* name;
  int version;
  short flags;
  void(*init)();
  void(*startio)();
  void(*unload)();
};

typedef void(*drentry)(struct drivobj* u); //just setup the drivobj

void* dralloc(drentry k){
  int* u = drivbrk;
  drivbrk += 6;
  *u = 0xFEEDC0DE;
  *(u + 1) = (int)k;
  return (void)(u + 1);
}

void drexec(drentry k){
  struct drivobj* u = TALLOC(struct drivobj);
  u->flags = 0;
  u->version = 1;
  k(u);
  u->init();
}

void drrun(void* u){
  if(u != 0 && *u != 0 && *(u-1) == 0xFEEDC0DE){
   drentry k = (drentry)(*u);
   drexec(k);
  }
}

void drfree(void* u){
  int* t = u;
  if(t != 0){
    *t = 0;
    *(t-1) = 0;
  }
}

void drstartio(drentry k){
  struct drivobj* u = TALLOC(struct drivobj);
  k(u);
  u->startio();
}
