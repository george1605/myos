#pragma once
#include "lib.c"
#include "smem.h"
#include "ioapic.h"
#define X86ENV 1
#define ARMENV 2
#define MAINENV 0x4BB0000
#define SHUTDOWN 0x20
#define RESTART 0x21
int lpid = 1;
int glsig = 0;
typedef enum
{
  UNUSED,
  EMBRYO,
  STARTED,
  PAUSED,
  KILLED,
  ZOMBIE
} prstate;
typedef void (*func)(int argc, char **argv);

void cons(int argc, char **argv)
{
  if (argc == 0)
    return;
}

struct proc
{
  prstate state;
  struct proc *parent;
  int pid;
  int ret;
  char *stack;
  char *name;
  int ssize;
  func f;
} tproc, kproc;

struct sleeplock
{
  struct spinlock lock;
  struct proc proc;
};

struct environ
{
  struct context *ctx;
  void *vars;
  int cpunum;
  int type;
};

struct sleeplock initsleep(struct proc u)
{
  struct sleeplock i;
  i.proc = u;
  acquire(&i.lock);
  return i;
}

void sleep(struct proc u)
{
  struct sleeplock f = initsleep(u);
  while (f.proc.state != KILLED && f.proc.stack != 0)
  { // waits to exit
  }
  release(&f.lock);
}

void endsleep(struct sleeplock u)
{
  release(&u.lock);
  u.proc.state = STARTED;
}

struct cpu *mycpu(void)
{
  int id = lapicid();
  int i = 0;
  while (cpus[i].cid != id && i < 32)
  {
    i++;
  }
  return &(cpus[i]);
}

struct proc myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli(c);
  c = mycpu();
  p = c->proc;
  popcli(c);
  return (*p);
}

void prcpy(struct proc a, struct proc b)
{ // copies the stack
  int n = 0;
  while (n < a.ssize && n < b.ssize)
  {
    *(b.stack++) = *(a.stack++);
    n++;
  }
}

void przom(struct proc a)
{
  a.state = ZOMBIE;
}

struct proc prdup(struct proc a)
{
  struct proc t;
  t.parent = a.parent;
  t.stack = a.stack;
  return t;
}

void prinit(struct proc u, struct proc *parent)
{
  u.pid = (++lpid);
  u.state = STARTED;
  u.stack = alloc(0, 64);
  if (parent == (struct proc *)0)
    u.parent = &tproc;
  else
    u.parent = parent;
  tproc = u;
}

struct proc prcreat(char *prname)
{
  struct proc u;
  prinit(u, 0);
  u.name = prname;
}

struct proc proc_init()
{
  struct proc t;
  t.state = STARTED;
  t.pid = 1;
  t.stack = kalloc(100, KERN_MEM);
  t.f = cons;
  kproc = t;
  kproc.f(0, (char **)0);
}

void proc_exit()
{
  HALT();
  kproc.state = KILLED;
  kproc.parent = (struct proc *)0;
  free((int *)kproc.stack);
  kproc.ssize = 0;
  glsig = SHUTDOWN;
}

void prswap(struct proc u)
{
  HALT();
  tproc = u;
  u.f(0, (char **)0);
}

void prsswap(struct proc prlist[], int procs)
{
  int a = 0;
  struct proc t = prlist[a];
  while (a < procs)
  {
    if (t.state != KILLED)
      prswap(t);

    t = prlist[++a];
  }
}

void prkill(struct proc u)
{
  if (u.pid > 1 && u.parent != 0)
  { // is a valid process
    HALT();
    prswap(*u.parent);
    u.state = KILLED;
    u.pid = 0;
    free((int *)u.stack);
  }
}

void prend(struct proc u, int status)
{
  prkill(u);
  kprint("Process ended with status: ");
}

void exitk()
{ // enters the kernel
  prkill(tproc);
  prswap(kproc);
}

void dbgret(int code)
{
  tproc.ret = code;
  exitk();
}

void prexec(int argv, char **argc)
{
  if (tproc.state != KILLED)
    tproc.f(argv, argc);
}

void pushctx(struct context *ctx)
{
  if (ctx != 0)
  {
    asm volatile("movl %1, %%ebp"
                 : "=r"(ctx->ebp)
                 : "r"(ctx->ebp));
    asm volatile("movl %1, %%ebx"
                 : "=r"(ctx->ebx)
                 : "r"(ctx->ebx));
    asm volatile("movl %1, %%edi"
                 : "=r"(ctx->edi)
                 : "r"(ctx->edi));
    asm volatile("movl %1, %%esi"
                 : "=r"(ctx->esi)
                 : "r"(ctx->esi));
  }
}

void readctx(struct context *ctx)
{
  if (ctx != 0)
  {
    asm volatile("movl %%ebp, %1"
                 : "=r"(ctx->ebp)
                 : "r"(ctx->ebp));
    asm volatile("movl %%ebx, %1"
                 : "=r"(ctx->ebx)
                 : "r"(ctx->ebx));
    asm volatile("movl %%edi, %1"
                 : "=r"(ctx->edi)
                 : "r"(ctx->edi));
    asm volatile("movl %%esi, %1"
                 : "=r"(ctx->esi)
                 : "r"(ctx->esi));
  }
}

void envinit(struct environ *u)
{
  u->cpunum = mycpu()->cid;
  u->vars = kalloc(32, KERN_MEM);
  u->type = X86ENV;
  u->ctx = kalloc(sizeof(struct context), KERN_MEM);
  memcpy(u->vars, "PATH=/home\n", 12);
  readctx(u->ctx);
}

void envpush(struct environ *u)
{
  pushctx(u->ctx);
}

void envswap(struct environ *u, int cid)
{
  u->cpunum = cid;
}

void envfree(struct environ *u)
{
  free(u->ctx);
  free(u->vars);
  u->vars = NULL_PTR;
}

void envrun(struct environ *u, func f)
{
  if (u == NULL_PTR)
    return;

  f(1, (char **)u);
}

struct isolate
{
  char *stack;
  int files[8];
  int fcnt;
  int (*handler)(int);
};

struct isolate islcreat(int (*x)(int))
{
  struct isolate u;
  u.fcnt = 0;
  u.stack = kalloc(128, USER_MEM);
  memset(u.files, 0, 8);
  u.handler = x;
  return u;
}

int __isldef(int code)
{
  char *stk;
  if (code != 0)
  {
    stk = ((struct isolate *)code)->stack;
    *(stk + 1) = 0;
  }
  return 0;
}

struct isolate islcreatx(int (*x)(int), size_t ssize)
{
  struct isolate u;
  u.stack = kalloc(ssize, USER_MEM);
  memset(u.files, 0, 8);
  u.handler = x;
  return u;
}

int islrun(struct isolate i)
{
  if (i.stack == 0)
    i.stack = kalloc(128, USER_MEM);

  int p = i.handler((int)&i);
  free(i.stack);
  return p;
}
