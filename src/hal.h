#pragma once

#ifndef ESP32
#include <stm32f4xx_hal.h>
#include <stm32f407xx.h>
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
