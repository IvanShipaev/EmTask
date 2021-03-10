//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

#ifndef __VFP_FP__
	#error Error - Hardware floating point support not enable.
#endif

#define portNVIC_SYSPRI2		((volatile unsigned*)0xe000ed20)
#define portNVIC_PENDSV_PRI	    (cfgKERNEL_LOW_PRI << 16)
#define portNVIC_SYSTICK_PRI	(cfgKERNEL_LOW_PRI << 24)

#define portNVIC_INT_CTRL		((volatile unsigned*)0xe000ed04)
#define portNVIC_PENDSVSET		(0x10000000)

// FPU
#define portFPU_CPACR			((volatile unsigned*)0xe000ed88)
#define portFPU_FPCCR			((volatile unsigned*)0xe000ef34)

static unsigned count_critical_task = 0x7fffffff;
static unsigned count_critical_irq = 0x7fffffff;

//-------------------------------------------------------------
// Interrupt functions
//-------------------------------------------------------------
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
//-------------------------------------------------------------
// Disable/enable systems priority ISR
//-------------------------------------------------------------
void EM_DISABLE_TASK(void)
{
	__set_BASEPRI(cfgKERNEL_MAX_PRI);
	count_critical_task++;
}
void EM_ENABLE_TASK(void)
{
	if ((count_critical_task == 0) || (--count_critical_task == 0))
		__set_BASEPRI(0);
}
//-------------------------------------------------------------
// Disable/enable systems all systems ISR
//-------------------------------------------------------------
void EM_DISABLE_ISR(void)
{
	__disable_irq();
	++count_critical_irq;
}
void EM_ENABLE_ISR(void)
{
	if ((count_critical_irq == 0) || (--count_critical_irq == 0))
		__enable_irq();
}
//--------------------------------------------------------------
//--------------------------------------------------------------
unsigned* rt_em_port_init_stack(unsigned* sp, unsigned size_sp,
		void (*start_func)(void*), void* par, void (*exit_func)(void))
{
	// Initialization stack
	for (int i = 0; i < size_sp; i++)
		*sp++ = cfgFILL_SP;
	--sp;
	*sp = 0x01000000; 			// xPSR
	--sp;
	*sp = (unsigned) start_func;
	--sp;
	*sp = (unsigned) exit_func; // LR
	sp -= 5; 					// R12, R3, R2, R1
	*sp = (unsigned) par; 		// R0
	--sp;
	*sp = 0xfffffffd;			// FPU register not used by this context
	sp -= 8; 					// R11, R10, R9, R8, R7, R6, R5, R4
	*sp = 0;
	return sp;
}
//--------------------------------------------------------------
static void __port_init_sys_timer(void)
{
	if (SysTick_Config(cfgCPU_CLOCK_HZ / cfgTICK_RATE_HZ))
		while (1);
}
//--------------------------------------------------------------
static void __rt_em_port_start_first_task(void)
{
	__asm volatile
	(
		" ldr 	r0, =0xE000ED08		\n"
		" ldr 	r0, [r0]			\n"
		" ldr 	r0, [r0]			\n"
		" msr 	msp, r0	    		\n"

		" mov r0, #0				\n"	// Clear bit FPU is in use
		" msr control, r0			\n"

		" cpsie	i					\n"
		" cpsie	f					\n"
		" svc 	0					\n"
		" nop						\n"
	);
}
//--------------------------------------------------------------
void rt_em_port_start_first_task(void)
{
	count_critical_task = 0;
	count_critical_irq = 0;
	__port_init_sys_timer();
	*(portNVIC_SYSPRI2) |= portNVIC_PENDSV_PRI;
	*(portNVIC_SYSPRI2) |= portNVIC_SYSTICK_PRI;

	// Enable CP10 and CP11
	*(portFPU_CPACR) |= 0x0fUL << 20;
	// Set ASPEN and LSPEN bits
	*(portFPU_FPCCR) |= 0x03UL << 30;

	__rt_em_port_start_first_task();
}
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
// ISR
//--------------------------------------------------------------
// SVC ISR
//--------------------------------------------------------------
void SVC_Handler(void)
{
	__asm volatile
	(
		" ldr	r3, =ctask			\n"
		" ldr 	r1, [r3]			\n"
		" ldr 	r0, [r1]			\n"
		" ldmia r0!, {r4-r11, r14}	\n"
		" msr 	psp, r0				\n"
		" mov 	r0, #0				\n"
		" msr	basepri, r0			\n"
		" bx 	r14					\n"
	);
}
//--------------------------------------------------------------
// PendSV ISR
//--------------------------------------------------------------
void PendSV_Handler(void)
{
	__asm volatile
	(
		" mrs   r0, psp					\n"

		" ldr 	r3, =ctask		    	\n" // Get SP for current task
		" ldr 	r2, [r3]				\n" // Get stack

		" tst r14, #0x10				\n" // Is using FPU context?
		" it eq							\n"
		" vstmdbeq r0!, {s16-s31}		\n" // Save FPU context

		" stmdb	r0!, {r4-r11, r14}	   	\n" // Save registers
		" str 	r0, [r2]				\n"

		" stmdb sp!, {r3}		   		\n"
		" mov   r0, %0      			\n" // Up base priority
		" msr 	basepri, r0		    	\n"
		" bl 	rt_em_switch_context	\n" // Change task
		" mov 	r0, #0			    	\n"
		" msr 	basepri, r0		    	\n" // Down base priority
		" ldmia	sp!, {r3}		   		\n"

		" ldr 	r1, [r3]				\n" // Get SP for current task
		" ldr 	r0, [r1]				\n"

		" ldmia r0!, {r4-r11, r14}	   	\n" // Load registers

		" tst r14, #0x10				\n" // Is using FPU context?
		" it eq							\n"
		" vldmiaeq r0!, {s16-s31}		\n" // Load FPU context

		" msr 	psp, r0			    	\n"
		" bx 	r14						\n"
		:: "i" (cfgKERNEL_MAX_PRI)
	);
}
//--------------------------------------------------------------
// System timer ISR
//--------------------------------------------------------------
void SysTick_Handler(void)
{
	rt_em_isr_system_tic();
}
//--------------------------------------------------------------
int rt_em_find_first_bit(int value)
{
	__asm volatile ( "clz	%0, %0" : "=r" (value) );
	return 31 - value;
}
//--------------------------------------------------------------
#if cfgTEST_SP_SWITCH_TASK
void rt_em_port_test_sp_switch_task(void)
{
	extern void EM_TEST_SP_SWITCH_HOOK(const char* task_name);
	unsigned* p = ctask->sp_start;
	if (ctask->sp < &p[20]) {
		EM_TEST_SP_SWITCH_HOOK(ctask->name);
		while (1)
			;
	} else {
		for (int i = 0; i < 20; i++) {
			if (p[i] != cfgFILL_SP) {
				EM_TEST_SP_SWITCH_HOOK(ctask->name);
				while (1)
					;
			}
		}
	}
}
#endif /* cfgTEST_SP_SWITCH_TASK */
//--------------------------------------------------------------
void rt_em_port_request_switch_context(void)
{
	*(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;
}
//--------------------------------------------------------------
void rt_em_port_test_sp(em_list_t* list_task)
{
	int len;
	unsigned* p;
	em_task_t *task;
	em_list_t *lst;

	EM_PRINTF("****************Test SP****************\r\n");
	lst = list_task->next;
	while (lst != list_task) {
		task = rt_em_get_struct_by_field(lst, em_task_t, init_list);
		for (p = task->sp_start; p < task->sp_start + task->sp_size; p++)
			if (*p != cfgFILL_SP)
				break;
		len = p - task->sp_start;
		EM_PRINTF("Task %s: sp %d state %d pri %d\r\n", task->name, len,
				task->state, task->pri);
		lst = lst->next;
	}
	EM_PRINTF("Free Heap space %u\r\n", em_heap_get_free_space());
	EM_PRINTF("***************************************\r\n");
}
