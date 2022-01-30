#pragma once
#define FL_IF 0x00000200 
#define ERR_INTRON 0x3A
#define ERR_UNKNOWNAPIC 0x3B
#include "lib.c"
//emulates the CPU ;)
struct _cpu {
  char regs[4];
  int pc;
} cpu;

void movax(char val){
  cpu.regs[0] = val;
}

void movbx(char val){
  cpu.regs[1] = val;
}

void cpuinc(){
  cpu.pc++;
}

void cpudec(){
 if(cpu.pc > 0)
   cpu.pc--;
}

void cpuclr(){
 cpu.regs = {0,0,0,0};
 cpu.pc = 0;
}

#define cpuid(level, a, b, c, d)                        \
  asm ("cpuid\n\t"                                        \
           : "=a" (a), "=b" (b), "=c" (c), "=d" (d)        \
           : "0" (level))

#define cpuid_count(level, count, a, b, c, d)                \
  asm ("cpuid\n\t"                                        \
           : "=a" (a), "=b" (b), "=c" (c), "=d" (d)        \
           : "0" (level), "2" (count))

#define is_x64() 
#define is_x86()

void cpuctl(int id, int req, int *params)
{
  switch (req)
  {
    case 0:
     if(params == 0) return;
     cpuid(params[0], params[1], params[2], params[3], params[4]);
    break;
    case 1:
     
    break;
    default:
     raise(-1);
    break;
  }
}
