#pragma once
#include "lib.c"
#include "process.h"
#include "port.h"
#include "mem.h"
#include "time.c"
#include "fs.h"
#include "vfs.h"
#include "user.h"
#include "pci.h"
#define PORTMAX 0xFFFF
#define SYSRES 0xC0DEBA5E /* system reserved space */
#define PM1a_CNT 0x4004
#define SLP_TYPa 0
#define SLP_EN 0
int usermode = 0;
int sysmode = 1;

#if _POSIX >= 2
#define POSIX_ARG_MAX 4096
#define POSIX_LINK_MAX 8
#define POSIX_MAX_CANON 255
#define POSIX_MAX_INPUT 255
#define POSIX_NAME_MAX 14
#define POSIX_PIPE_BUF 512
#define POSIX_SSIZE_MAX 32767
#define POSIX_STREAM_MAX 8
#endif
#if _POSIX >= 3
#define POSIX_PATH_MAX 4096
#define POSIX_USERNAME_MAX 32
#define POSIX_SSIZE_MAX 32767
#define POSIX_PIPE_BUF 512
#define POSIX_PIPEX_BUF 4096
#endif

struct eptr
{
  void *ebp;
  void *esp;
};

int sys_int(void *arg1)
{
  if (arg1 != 0)
    return *(int *)arg1;
}

void sys_sleep()
{
  outw(PM1a_CNT, SLP_TYPa | SLP_EN);
}

struct rtcdate *sys_time()
{
  struct rtcdate *u = (struct rtcdate *)0xDDFF00;
  filldate(u);
  return u;
}

void sys_exec(void *arg1, void *arg2)
{
  if (arg1 != 0)
  {
    prexec(*(int *)arg1, (char **)arg2);
  }
  else
  {
    prexec(0, (char **)arg2);
  }
}

void sys_open(void *arg1, void *arg2)
{
  int mode = sys_int(arg2);
  struct file f = opfile((char *)arg1);
  f.flags = mode;
}

void sys_mkdir(void *arg1, void *arg2)
{
  char *dname = (char *)arg1;
  struct file *parent = (struct file *)arg2;
  mkdir(dname, parent);
}

void sys_write(void *arg1, void *arg2)
{
  struct buf *a = (struct buf *)arg2;
  a->flags = B_DIRTY;
  _write(sys_int(arg1), a, 512);
}

void sysc_handler(struct regs *r)
{
  switch (r->eax)
  {
  case 0x4:
    sys_open((void *)r->ebx, (void *)r->ecx);
    break;
  case 0x5:
    sys_write((void *)r->ebx, (void *)r->ecx);
    break;
  case 0xF:
    r->ebx = (size_t)sys_time();
    break;
  case 0x10:
    sys_sleep();
    break;
  case 0x11:
    r->err_code = inb(r->ebx);
    break;
  case 0x12:
    outb(r->ebx, r->err_code);
    break;
  }
}

void sysc_load() // add the sysc_handler()
{
  idt_set_gate(0x80, (unsigned)sysc_handler, 0x08, 0x8F);
}

void switch_userm()
{
  asm volatile("  \
     cli; \
     mov $0x23, %ax; \
     mov %ax, %ds; \
     mov %ax, %es; \
     mov %ax, %fs; \
     mov %ax, %gs; \
                   \
     mov %esp, %eax; \
     pushl $0x23; \
     pushl %eax; \
     pushf; \
     pushl $0x1B; \
     push $1f; \
     iret; \
   1: \
     ");
  usermode = 1;
  sysmode = 0;
}

void switch_kernm()
{
  usermode = 0;
  sysmode = 1;
}

void asm_syscall(uint64_t a)
{
  asm volatile("syscall");
}

struct pollfd
{
  int fd;
  short events;
  short revents;
};

void sys_poll()
{
}

struct rsdp_desc
{
  char sign[8];
  uint8_t checksum;
  char OEMID[6];
  uint8_t revision;
  uint32_t raddr;
} __attribute__((packed));

struct pres
{
  union
  {
    int fd;
    long devfd;
    void *buffer;
  } value;
  int type;
};

struct file getfile(char *name)
{
  struct file u;
  return u;
}

void sys_abort()
{
  prkill(tproc);
}

uint32_t sys_memsz()
{ // using CMOS
  uint32_t total;
  uint16_t lowmem, highmem;

  outportb(0x70, 0x30);
  lowmem = inportb(0x71);
  outportb(0x70, 0x31);
  highmem = inportb(0x71);

  total = lowmem | highmem << 8;
  return total;
}

void setcore(uint64_t base)
{
  asm volatile("wrmsr"
               :
               : "c"(0xc0000101), "d"((uint32_t)(base >> 32)), "a"((uint32_t)(base & 0xFFFFFFFF)));
  asm volatile("wrmsr"
               :
               : "c"(0xc0000102), "d"((uint32_t)(base >> 32)), "a"((uint32_t)(base & 0xFFFFFFFF)));
}

struct proc sys_execv(char *path, char *argv[])
{
  int argc = 0;
  while (argv[argc] != 0)
  {
    argc++;
  }
  struct proc u = prcreat(path);
  u.f(argc, (char **)argv);
  return u;
}

#define GETPROC(x) *(struct proc *)(x[0]) // gets an proc structure from char**

void _waitfunc(int argc, char **argv)
{
  struct proc i = GETPROC(argv);
  if (argc == 1)
  {
    while (1)
    {
      if (i.state == PAUSED)
        break;
    }
  }
}

struct rmentry
{
  void *ptr;
  int size;
};

struct _rmem
{
  struct rmentry entries[16];
  int n;
} rmtable;

void sys_prwait(struct proc u)
{
}

void sys_resmem(void *mem, int size)
{
  struct rmentry i;
  int *ptr = mem;
  if ((int)ptr > 1 && rmtable.n < 16)
  {
    *(ptr - 1) = SYSRES;
    *(ptr + size) = 0;
    i.ptr = ptr;
    i.size = size;
    rmtable.entries[++rmtable.n] = i;
  }
}

void restxt()
{ // reserves the text buffer at 0xB8000
  sys_resmem((void *)0xB8000, 80 * 25 + 1);
}

void resvid()
{ // reserves the video memory
  sys_resmem((void *)0xA0000, 200 * 320 + 1);
}
