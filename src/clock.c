/*
This module configures the internal clock system and enables the PLL for the 48MHz system clock
*/

#include "stm32f0xx.h"
void internal_clock()
{
    //disable HSE to allow use of the GPIOs
    RCC->CR &= ~RCC_CR_HSEON;
    //enable prefetch buffer and set flash Latency
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    //HCLK = SYSCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
    //PCLK = HCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;
    //PLL configuration = (HSI/2) * 12 = ~48 MHz
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLXTPRE_HSE_PREDIV_DIV1 | RCC_CFGR_PLLMUL12);
    //enable the PLL
    RCC->CR |= RCC_CR_PLLON;
    //wait until the PLL is ready
    while((RCC->CR & RCC_CR_PLLRDY) == 0);
    //select PLL as system clock source
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;
    //wait until PLL is used as system clock source
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL);
    FLASH->ACR |= FLASH_ACR_LATENCY;
}