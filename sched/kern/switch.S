/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.

 */

.globl context_switch;
	context_switch:
	mov 4(%esp), %eax;
	mov %eax, %esp;
	popal;

	popl %es;
	popl %ds;

	addl $0x8, %esp;
	iret;
	
spin:
	jmp spin
