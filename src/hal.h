#pragma once

#include <stdint.h>

#ifndef ESP32
#include <stm32g0xx_hal.h>
#else
#include <Arduino.h>
#endif

namespace JJL
{
    static uint32_t sysMillis();
    static uint32_t sysMicros();
}

#include "hal.hpp"