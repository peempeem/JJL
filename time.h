#pragma once

#include <stdint.h>

#ifdef ARDUINO_H

#include <Arduino.h>

static inline uint32_t sysMicros()
{
    return micros();
}

static inline uint32_t sysMillis()
{
    return millis();
}

#else

#include "../../../Drivers/CMSIS/Include/core_cm0.h"

extern volatile uint32_t UptimeMillis;

static uint32_t sysMicros()
{
    uint32_t ms;
    uint32_t st;

    // Read UptimeMillis and SysTick->VAL until
    // UptimeMillis doesn't rollover.
    do
    {
        ms = UptimeMillis;
        st = SysTick->VAL;
    } while (ms != UptimeMillis);

    return ms * 1000 - st / ((SysTick->LOAD + 1) / 1000);
}

static inline uint32_t sysMillis()
{
    return UptimeMillis;
}

#endif

#ifndef RATE_H
#define RATE_H

class Rate {
    public:
        Rate();
        Rate(float rate);

        void    setRate(float rate);
        bool    isReady();
        void    reset();
        void    enable();
        void    disable();
        float   getStage(bool noChange=false);

    private:
        uint64_t    _inverseRate = 0;
        uint64_t    _last = 0;
        bool        _enabled = false;
};

#endif
