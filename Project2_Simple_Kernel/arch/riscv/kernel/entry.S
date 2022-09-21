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
  //addi sp, sp, -(SWITCH_TO_SIZE)

  /* TODO: [p2-task1] save all callee save registers on kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  ld t0, PCB_KERNEL_SP(a0)
  addi t0, t0, -(SWITCH_TO_SIZE)
  sd  ra, SWITCH_TO_RA  (t0)
  sd  sp, SWITCH_TO_SP  (t0)
  sd  s0, SWITCH_TO_S0  (t0)
  sd  s1, SWITCH_TO_S1  (t0)
  sd  s2, SWITCH_TO_S2  (t0)
  sd  s3, SWITCH_TO_S3  (t0)
  sd  s4, SWITCH_TO_S4  (t0)
  sd  s5, SWITCH_TO_S5  (t0)
  sd  s6, SWITCH_TO_S6  (t0)
  sd  s7, SWITCH_TO_S7  (t0)
  sd  s8, SWITCH_TO_S8  (t0)
  sd  s9, SWITCH_TO_S9  (t0)
  sd s10, SWITCH_TO_S10 (t0)
  sd s11, SWITCH_TO_S11 (t0)
  sd t0, PCB_KERNEL_SP(a0)

  /* TODO: [p2-task1] restore all callee save registers from kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  ld t0, PCB_KERNEL_SP(a1)
  ld  ra, SWITCH_TO_RA  (t0)
  ld  sp, SWITCH_TO_SP  (t0)
  ld  s0, SWITCH_TO_S0  (t0)
  ld  s1, SWITCH_TO_S1  (t0)
  ld  s2, SWITCH_TO_S2  (t0)
  ld  s3, SWITCH_TO_S3  (t0)
  ld  s4, SWITCH_TO_S4  (t0)
  ld  s5, SWITCH_TO_S5  (t0)
  ld  s6, SWITCH_TO_S6  (t0)
  ld  s7, SWITCH_TO_S7  (t0)
  ld  s8, SWITCH_TO_S8  (t0)
  ld  s9, SWITCH_TO_S9  (t0)
  ld s10, SWITCH_TO_S10 (t0)
  ld s11, SWITCH_TO_S11 (t0)
  add t0, t0, SWITCH_TO_SIZE
  sd t0, PCB_KERNEL_SP(a1)

  mv tp, a1

  //addi sp, sp, SWITCH_TO_SIZE
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
