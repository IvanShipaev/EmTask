//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _TIMER_TASK_H_
#define _TIMER_TASK_H_

#if defined (cfgUSE_TIMER_TASK)

enum em_timer_type
{
	EM_TIMER_SINGLE = 0,
	EM_TIMER_REUSE = 1,
};

typedef struct _em_timer_t
{
	em_list_t timer_list;
	unsigned alltimeout;
	unsigned timeout;
	enum em_timer_type type;
	void (*func)(void *par);
	void *par;
} em_timer_t;

void em_timer_module_init (void);
void em_timer_init(em_timer_t* timer);
void em_timer_start(em_timer_t* timer, unsigned timeout,
		enum em_timer_type type, void (*func)(void *), void *par);
void em_timer_restart(em_timer_t* timer);
void em_timer_stop(em_timer_t* timer);

#ifdef _EM_TASK_PRIVATE_
void rt_timer_poll_wait_list(void);
#endif /* _EM_TASK_PRIVATE_ */

#endif /* cfgUSE_TIMER_TASK */

#endif /* _TIMER_TASK_H_ */
