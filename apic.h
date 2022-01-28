#pragma once
#include "lib.c"

uint64_t apicreadb()
{
    uint64_t low;
    uint64_t high;
    asm("rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(0x1B));

    return (high << 32) | low;
}

void apicwriteb(uint64_t val)
{
    uint64_t low = val & 0xFFFFFFFF;
    uint64_t high = val >> 32;
    asm("wrmsr" ::"a"(low), "d"(high), "c"(0x1B));
}
