/**
 *******************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * This software component is licensed by WCH under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */
#include "backup.h"
#include "clock.h"
#include "core_riscv_ch32yyxx.h"
#include "ch32yyxx_rcc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TICK_FREQ_1KHz    1L

__IO uint64_t msTick=0;
uint32_t SystemCoreClock_MHz_inv = 0;

WEAK uint64_t GetTick(void)
{
  uint32_t high1, high2, low;
  do {
    high1 = (uint32_t)(msTick >> 32);
    low   = (uint32_t)msTick;
    high2 = (uint32_t)(msTick >> 32);
  } while (high1 != high2);
  return ((uint64_t)high1 << 32) | low;
}

void osSystickHandler() __attribute__((weak, alias("noOsSystickHandler")));
void noOsSystickHandler()
{
}

/**
  * @brief  Function called to read the current millisecond
  * @param  None
  * @retval None
  */
uint32_t getCurrentMillis(void)
{
  return GetTick();
}

#if defined(CH32V10x)
  #define SYSTICK_CNTL    (0xE000F004)   
  #define SYSTICK_CNTH    (0xE000F008)
  #define SYSTICK_CMPL    (0xE000F00C)
  #define SYSTICK_CMPH    (0xE000F010)
  #define SYSTICK_GET_CNT() (*((__IO uint32_t *)SYSTICK_CNTL))
  #define CYCLES_PER_US   (SystemCoreClock / 8000000)
#else
  #define SYSTICK_GET_CNT() ((uint32_t)SysTick->CNT)
  #define CYCLES_PER_US   (SystemCoreClock / 1000000)
#endif

/**
  * @brief  Function called to read the current microsecond
  * @param  None
  * @retval None
  */
// Weak so a USB library can replace the timebase without changing non-USB builds.
WEAK uint32_t getCurrentMicros(void)
{
  // Optimize: Avoid 64-bit/32-bit software division (which takes ~200 cycles and bloats binary).
  // Precalculate the inverse scaling factor (1<<32) / cycles_per_us once at startup,
  // then use hardware multiplication (mulhu) and shift to compute microsecond fraction in ~10 cycles.
  if (SystemCoreClock_MHz_inv == 0) {
    SystemCoreClock_MHz_inv = (uint32_t)((1ULL << 32) / CYCLES_PER_US);
  }

  uint32_t m0 = (uint32_t)GetTick();
  uint32_t u0 = SYSTICK_GET_CNT();
  uint32_t m1 = (uint32_t)GetTick();
  uint32_t u1 = SYSTICK_GET_CNT();   //may be a interruption

  if (m1 != m0) {
    return m1 * 1000 + (uint32_t)(((uint64_t)u1 * SystemCoreClock_MHz_inv) >> 32);
  } else {
    return m0 * 1000 + (uint32_t)(((uint64_t)u0 * SystemCoreClock_MHz_inv) >> 32);
  }
}

#if defined(CH32V20x) || defined(CH32V30x) || defined(CH32V30x_C) || defined(CH32V00x) || defined(CH32X035) || defined(CH32L10x) || defined(CH32VM00X)
/*********************************************************************
 * @fn      SysTick_Handler
 *
 * @brief   This function handles systick interrupt.
 *
 * @return  none
 */
// Weak so rv003usb can use SysTick as a free-running counter only when linked.
void SysTick_Handler(void) __attribute__((weak, interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
  msTick+=TICK_FREQ_1KHz;
  osSystickHandler();
  SysTick->SR = 0;
}
#endif

#if defined (CH32V10x)
/*********************************************************************
 * @fn      SysTick_Handler
 *
 * @brief   This function handles systick interrupt.
 *
 * @return  none
 */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
  SysTick->CTLR=0;
  msTick+=TICK_FREQ_1KHz;
  SysTick->CNTL0=0;SysTick->CNTL1=0;SysTick->CNTL2=0;SysTick->CNTL3=0;
  SysTick->CNTH0=0;SysTick->CNTH1=0;SysTick->CNTH2=0;SysTick->CNTH3=0;
  SysTick->CTLR=0x1;
  osSystickHandler();
}
#endif

#ifdef __cplusplus
}
#endif
