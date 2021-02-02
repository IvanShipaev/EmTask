//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _PORT_H_
#define _PORT_H_

#define cfgKERNEL_LOW_PRI 	(cfgLIB_LOW_PRI << (8 - configPRIO_BITS))
#define cfgKERNEL_MAX_PRI 	(cfgLIB_MAX_PRI << (8 - configPRIO_BITS))

void EM_DISABLE_TASK(void);
void EM_ENABLE_TASK(void);
void EM_DISABLE_ISR(void);
void EM_ENABLE_ISR(void);

#ifdef _EM_TASK_PRIVATE_

void rt_em_port_start_first_task(void);
unsigned* rt_em_port_init_stack(unsigned* sp, unsigned size_sp, void (*start_func) (void*),
		void* par, void (*exit_func) (void));
int rt_em_find_first_bit(int val);
void rt_em_port_request_switch_context(void);
void rt_em_port_test_sp(em_list_t* list_task);

#endif /* _EM_TASK_PRIVATE_ */

#endif /*_PORT_H_*/
