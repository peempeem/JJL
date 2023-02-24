#pragma once

#ifndef ESP32
#include <stm32f0xx_hal.h>
#include <stdint.h>

static inline uint32_t sysMillis()
{
    return HAL_GetTick();
}

#else

#include <Arduino.h>
#include <stdint.h>

static inline uint32_t sysMillis()
{
    return millis();
}

#endif
