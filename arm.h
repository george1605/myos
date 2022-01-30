#pragma once
#include "lib.c"
#define is_arm() sizeof(long) == 4 && sizeof(int) == 4
#define achg(reg1,reg2) asm("xchg "reg1", "reg2)

int* armbrk = 0x2A0000;

void* armalloc(int bytes)
{
    void* u = armbrk;
    u += (bytes + 1);

}

void xrqinstall(size_t ndx, void* addr)
{
    size_t *v;
    v = (size_t*)0x0;
    v[ndx] = 0xEA000000 | (((size_t*)addr - 8 - (4 * ndx)) >> 2);
}

size_t cpsrget(){
    size_t r;
    asm("mrs %[ps], cpsr" : [ps]"=r" (r));
    return r;
}

void cpsrset(size_t r)
{
    asm("msr cpsr, %[ps]" : : [ps]"r" (r));
}
