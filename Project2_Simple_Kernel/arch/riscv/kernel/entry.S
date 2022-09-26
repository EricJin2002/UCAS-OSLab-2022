#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

.macro SAVE_CONTEXT
  /* TODO: [p2-task3] save all general purpose registers here! */
  /* HINT: Pay attention to the function of tp and sp, and save them carefully! */

  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  li t0, SR_SUM | SR_FS

  /* TODO: [p2-task3] save sstatus, sepc, stval and scause on kernel stack */

.endm

.macro RESTORE_CONTEXT
  /* TODO: Restore all general purpose registers and sepc, sstatus */
  /* HINT: Pay attention to sp again! */
.endm

ENTRY(enable_preempt)
  not t0, x0
  csrs CSR_SIE, t0
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
  csrw CSR_SIE, zero
  jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// NOTE: the address of previous pcb in a0
// NOTE: the address of next pcb in a1
ENTRY(switch_to)
  addi sp, sp, -(SWITCH_TO_SIZE)

  /* TODO: [p2-task1] save all callee save registers on kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  sd  ra, SWITCH_TO_RA  (sp)
  sd  sp, SWITCH_TO_SP  (sp)
  sd  s0, SWITCH_TO_S0  (sp)
  sd  s1, SWITCH_TO_S1  (sp)
  sd  s2, SWITCH_TO_S2  (sp)
  sd  s3, SWITCH_TO_S3  (sp)
  sd  s4, SWITCH_TO_S4  (sp)
  sd  s5, SWITCH_TO_S5  (sp)
  sd  s6, SWITCH_TO_S6  (sp)
  sd  s7, SWITCH_TO_S7  (sp)
  sd  s8, SWITCH_TO_S8  (sp)
  sd  s9, SWITCH_TO_S9  (sp)
  sd s10, SWITCH_TO_S10 (sp)
  sd s11, SWITCH_TO_S11 (sp)
  sd  sp, PCB_KERNEL_SP (a0)

  /* TODO: [p2-task1] restore all callee save registers from kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  ld  sp, PCB_KERNEL_SP (a1)
  ld  ra, SWITCH_TO_RA  (sp)
  ld  sp, SWITCH_TO_SP  (sp)
  ld  s0, SWITCH_TO_S0  (sp)
  ld  s1, SWITCH_TO_S1  (sp)
  ld  s2, SWITCH_TO_S2  (sp)
  ld  s3, SWITCH_TO_S3  (sp)
  ld  s4, SWITCH_TO_S4  (sp)
  ld  s5, SWITCH_TO_S5  (sp)
  ld  s6, SWITCH_TO_S6  (sp)
  ld  s7, SWITCH_TO_S7  (sp)
  ld  s8, SWITCH_TO_S8  (sp)
  ld  s9, SWITCH_TO_S9  (sp)
  ld s10, SWITCH_TO_S10 (sp)
  ld s11, SWITCH_TO_S11 (sp)

  mv tp, a1

  addi sp, sp, SWITCH_TO_SIZE
  jr ra
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  /* TODO: [p2-task3] restore context via provided macro and return to sepc */
  /* HINT: remember to check your sp, does it point to the right address? */
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)

  /* TODO: [p2-task3] save context via the provided macro */


  /* TODO: [p2-task3] load ret_from_exception into $ra so that we can return to
   * ret_from_exception when interrupt_help complete.
   */


  /* TODO: [p2-task3] call interrupt_helper
   * NOTE: don't forget to pass parameters for it.
   */


ENDPROC(exception_handler_entry)
