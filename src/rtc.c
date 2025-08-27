/*
This module simulates a real-time clock using SysTick for timestamp generation in each entry
*/

#include "rtc.h"
#include "stm32f0xx.h"

static uint32_t simulatedTime = 0;

void rtcInit(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}

uint32_t rtcGetTimestamp(void)
{
    return simulatedTime;
    simulatedTime++;
}

void rtcSetTimestamp(uint32_t timestamp)
{
    simulatedTime = timestamp;
}

void SysTick_Handler(void)
{
    simulatedTime++;
}