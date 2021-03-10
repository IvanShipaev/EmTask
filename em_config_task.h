//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_CONFIG_TASK_H_
#define _EM_CONFIG_TASK_H_

#include "board.h"

#define EM_MIN(a, b)  ((a) < (b) ? (a) : (b))
#define EM_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define EM_TIM_INFINITY	(~0)
#define EM_MAX_UNSIGNED_VALUE (~0)

#define ALLIGN_LEAST(size, allign) ((size) & ~((allign) - 1))
#define ALLIGN_LARGEST(size, allign) (((size) + (allign) - 1) & ~((allign) - 1))

#ifdef __NVIC_PRIO_BITS
	#define configPRIO_BITS       		__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       		4
#endif

#define cfgLIB_LOW_PRI					0x0f
#define cfgLIB_MAX_PRI					10

#define cfgSIZE_STACK_IDLE_TASK			130
#define cfgNUM_PRI 						__PRI_MAX
#define cfgMEM_ALLIGN 					8
#define cfgSIZE_HEAP_BUF 				(1024*40UL)
#define cfgCPU_CLOCK_HZ					( BOARD_MCK )
#define cfgTICK_RATE_HZ					( 1000 )
#define cfgLEN_TASK_NAME 				16
#define cfgUSE_IDLE_HOOK 				0
#define cfgUSE_SYSTIC_HOOK 				0
#define cfgSTATIC_SYSTEM_TASK			0
#define cfgUSE_TIMER_TASK				1
#define cfgUSE_ASSERT					1
#define EM_PRINTF(...)					DEB_PRINTF(DEBUG_ROOT, __VA_ARGS__)
#define cfgTEST_SP_SWITCH_TASK			0
#define cfgFILL_SP						0x55555555
// Timer task
#if cfgUSE_TIMER_TASK
	#define cfgSIZE_STACK_TIMER_TASK 	256
	#define cfgPRI_TIMER_TASK			(cfgNUM_PRI-1)
	#define cfgSTATIC_TIMER_TASK		0
#endif

#if (cfgUSE_ASSERT)
	void __em_assert_func(char* file, unsigned line);
	#define EM_ASSERT(a)	((a) ? (void)0 : __em_assert_func((char*)__func__, __LINE__))
#endif

#endif /* _EM_CONFIG_TASK_H_ */
