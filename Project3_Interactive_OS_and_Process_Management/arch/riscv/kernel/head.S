/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK		0x50500000
#define PAGE_SIZE       4096

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  // for [p3-task3]
  // only main core will clear BSS
  csrr t0, CSR_MHARTID
  bnez t0, clear_bss_done

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

  // for [p3-task3]
  csrr t0, CSR_MHARTID
  beq t0, zero, init_sp_done
  li t1, -PAGE_SIZE
init_sp:
  add sp, sp, t1
  addi t0, t0, -1
  bnez t0, init_sp
init_sp_done:

  j main

loop:
  wfi
  j loop

END(_start)
