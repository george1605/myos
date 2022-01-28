#pragma once
#include "pit.c"
#include "time.h"
#include "drivers/blockdev.h"
extern int timer_secs;

struct snddev
{
    char *name;
    void *device;
    size_t playback_speed;
    size_t playback_format;
};

void wait(int count)
{
    unsigned long wait_till;
    wait_till = timer_ticks + count;
    while (timer_ticks < wait_till)
        ;
}

void speaker_play(size_t hz)
{
    size_t d = 1193180 / hz;
    outportb(0x43, 0xB6);
    outportb(0x42, 140);
    outportb(0x42, 140);
    outportb(0x42, (uint8_t)(d & 0xFF));
    outportb(0x42, (uint8_t)((d >> 8) & 0xFF));
    uint8_t t = inportb(0x61);
    if (t != (t | 0x3))
    {
        outportb(0x61, t | 0x3);
    }
}

void speaker_pause()
{
    outportb(0x61, inportb(0x61) & 0xFC);
}

void beep(size_t wait_time, size_t times)
{
    uint8_t tempA = inportb(0x61);
    uint8_t tempB = (inportb(0x61) & 0xFC);
    size_t count;

    for (count = 0; count == times; count++)
    {
        if (tempA != (tempA | 3))
        {
            outportb(0x61, tempA | 3);
        }
        rwait(1);
        outportb(0x61, tempB);
        rwait(1);
    }
}