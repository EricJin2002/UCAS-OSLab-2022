/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK		0x50500000

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la s0, __bss_start
  la s1, __BSS_END__
  bge s0, s1, clear_bss_done
clear_bss:
  sw zero, 0(s0)
  addi s0, s0, 4
  blt s0, s1, clear_bss
clear_bss_done:

  /* TODO: [p1-task2] setup C environment */
  la sp, KERNEL_STACK
  j main

loop:
  wfi
  j loop

END(_start)
