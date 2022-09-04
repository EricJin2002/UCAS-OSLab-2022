#include <asm/biosdef.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50201000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
	li a0, BIOS_PUTSTR
	la a1, msg
	call bios_func_entry

	// TODO: [p1-task2] call BIOS to read kernel in SD card
	li a0, BIOS_SDREAD
	la a1, kernel
	li s0, os_size_loc
	lh a2, 0(s0)
	li a3, 1
	call bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel
	// load task-info (i.e. app-info) block
	li a0, BIOS_SDREAD
	la a1, 0x52000000 // app address
	li s0, os_size_loc
	lh a2, -2(s0)
	lh a3, -4(s0)
	call bios_func_entry

	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	j kernel


// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

