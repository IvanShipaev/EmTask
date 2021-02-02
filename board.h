#ifndef _BOARD_H_
#define _BOARD_H_

#include "main.h"
#include "xconfig.h"
#include "debug_deb.h"
#define BOARD_MCK 		(84000000UL)

enum {
	__PRI_TASK_TEST 	= 1,
	__PRI_TASK_LCD		= 2,
	__PRI_TASK_WS2815 	= 3,
	//--------------
	__PRI_MAX
};

#ifndef sizeof_m
	#define sizeof_m(a) (sizeof(a)/sizeof((a)[0]))
#endif

#endif /* _BOARD_H_ */
