#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)

  /* TODO: [p2-task3] save exception_handler_entry into STVEC */
  la t0, exception_handler_entry
  csrw CSR_STVEC, t0

  /* TODO: [p2-task4] enable interrupts globally */
  // addi sp, sp, -8
  // sd ra, (sp)
  // call enable_interrupt
  // ld ra, (sp)
  // addi sp, sp, 8

  // why should we enable interrupts here?
  not t0, x0
  csrs CSR_SIE, t0

  jr ra

ENDPROC(setup_exception)
