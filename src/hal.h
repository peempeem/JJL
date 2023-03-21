#pragma once

#include <stdint.h>

#ifndef ESP32
#include <stm32f4xx_hal.h>

static inline uint32_t sysMillis()
{
    return HAL_GetTick();
}

static inline uint32_t sysMicros()
{
    uint32_t st = SysTick->VAL;
    uint32_t pending = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
    uint32_t ms = HAL_GetTick();

    if (pending == 0)
        ms++;

    return ms * 1000 - st / ((SysTick->LOAD + 1) / 1000);
}

#else
#include <Arduino.h>

static inline uint32_t sysMillis()
{
    return millis();
}

static inline uint32_t sysMicros()
{
    return micros();
}

#endif