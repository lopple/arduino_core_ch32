/* SPDX-License-Identifier: MIT */
// Minimal ch32fun compatibility shim for cnlohr/rv003usb.
// Derived from the public ch32v003fun/ch32fun interface used by rv003usb:
// https://github.com/cnlohr/ch32v003fun
// This is not a full ch32fun.h copy; C code uses this Arduino core's
// lib/ch32yyxx.h, while assembly keeps only the constants and XW macros
// required by rv003usb.S.

#ifndef _CH32FUN_H
#define _CH32FUN_H

#ifdef __ASSEMBLER__
// Provide minimal base addresses for assembly compilation without requiring full system headers
#define RCC_BASE      0x40021000
#define EXTI_BASE     0x40010400
#define GPIOA_BASE    0x40010800
#define GPIOC_BASE    0x40011000
#define GPIOD_BASE    0x40011400
#define TIM1_BASE     0x40012c00

#define GPIO_A_BASE   GPIOA_BASE
#define GPIO_C_BASE   GPIOC_BASE
#define GPIO_D_BASE   GPIOD_BASE

// XW custom instructions assembly encoder macros
#define ASM_ASSERT(COND) .if (!(COND)); .err; .endif

#define C_s0 0
#define C_s1 1
#define C_a0 2
#define C_a1 3
#define C_a2 4
#define C_a3 5
#define C_a4 6
#define C_a5 7

#define REG2I(X) (C_ ## X)

#define XW_OP_LBUSP 0b1000000000000000
#define XW_OP_STSP  0b1000000001000000
#define XW_OP_LHUSP 0b1000000000100000
#define XW_OP_SHSP  0b1000000001100000

#define XW_OP_LBU   0b0010000000000000
#define XW_OP_SB    0b1010000000000000
#define XW_OP_LHU   0b0010000000000010
#define XW_OP_SH    0b1010000000000010

#define XW_ENCODE1(OP, R1, R2, IMM) ASM_ASSERT((IMM) >= 0 && (IMM) < 32); .2byte ((OP) | (REG2I(R1) << 2) | (REG2I(R2) << 7) | \
	(((IMM) & 0b1) << 12) | (((IMM) & 0b110) << (5 - 1)) | (((IMM) & 0b11000) << (10 - 3)))

#define XW_ENCODE2(OP, R1, R2, IMM) ASM_ASSERT((IMM) >= 0 && (IMM) < 32); .2byte ((OP) | (REG2I(R1) << 2) | (REG2I(R2) << 7) | \
	(((IMM) & 0b11) << 5) | (((IMM) & 0b11100) << (10 - 2)))

#define XW_C_LBU(RD, RS, IMM) XW_ENCODE1(XW_OP_LBU, RD, RS, IMM)
#define XW_C_SB(RS1, RS2, IMM) XW_ENCODE1(XW_OP_SB, RS1, RS2, IMM)
#define XW_C_LHU(RD, RS, IMM) ASM_ASSERT(((IMM) & 1) == 0); XW_ENCODE2(XW_OP_LHU, RD, RS, ((IMM) >> 1))
#define XW_C_SH(RS1, RS2, IMM)  ASM_ASSERT(((IMM) & 1) == 0); XW_ENCODE2(XW_OP_SH, RS1, RS2, ((IMM) >> 1))

#else
// C compiler has access to all include paths, so safely include the core header
#include "lib/ch32yyxx.h"

// Compatibility macros for rv003usb.c using ch32fun names
#define GPIO_CFGLR_OUT_50Mhz_PP     3
#define GPIO_Speed_In               0
#define GPIO_CNF_IN_FLOATING        4

#define FLASH_KEY1                  ((uint32_t)0x45670123)
#define FLASH_KEY2                  ((uint32_t)0xCDEF89AB)
#define CR_LOCK_Set                 ((uint32_t)0x00000080)

#endif
#endif
