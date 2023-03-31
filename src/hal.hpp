#include "hal.h"

static uint32_t JJL::sysMillis()
{
    #ifndef ESP32
    return HAL_GetTick();
    #else
    return millis();
    #endif
}

static uint32_t JJL::sysMicros()
{
    #ifndef ESP32
    uint32_t st = SysTick->VAL;
    uint32_t pending = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
    uint32_t ms = HAL_GetTick();

    if (pending == 0)
        ms++;

    return ms * 1000 - st / ((SysTick->LOAD + 1) / 1000);
    #else
    return micros();
    #endif
}