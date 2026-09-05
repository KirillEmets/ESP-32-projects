#pragma once
#include <stdint.h>

typedef void (*TimerCallback)(void *arg);

typedef struct
{
    void (*setup)(TimerCallback);
    void (*start)(uint64_t duration_us);
} TimerVariant;

TimerVariant getSoftwareVariant();
TimerVariant getHardwareVariant();