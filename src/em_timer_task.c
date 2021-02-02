#define _TIMER_TASK_
#define _EM_TASK_PRIVATE_
#include "task_include.h"
#include "em_timer_task.h"

#if (cfgUSE_TIMER_TASK)

#define EM_TIMER_FLAG	(1 << 0)

static __EM_LIST_INIT(timer_wait_list);
static __EM_LIST_INIT(timer_ready_list);
static em_event_t timer_event[1];

//--------------------------------------------------------------
static inline em_timer_t* rt_em_get_timer_by_timer_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_timer_t, timer_list);
}
//--------------------------------------------------------------
static void __del_timer_wait_list(em_timer_t* timer)
{
	em_timer_t* t_timer;

	if (rt_em_is_list_empty(&timer->timer_list) == 0) {
		if (timer->timer_list.next != timer->timer_list.prev) {
			t_timer = rt_em_get_timer_by_timer_list(timer->timer_list.next);
			t_timer->timeout += timer->timeout;
		}
		rt_em_list_remove_entry(&timer->timer_list);
	}
}
//--------------------------------------------------------------
static int __add_timer_wait_list(em_timer_t* timer, unsigned timeout)
{
	em_list_t* list;
	em_timer_t* t_timer;

	if (timeout == 0) {
		timer->timeout = 0;
		rt_em_list_add_tail(&timer_ready_list, &timer->timer_list);
		return 1;
	}

	list = timer_wait_list.next;
	while (list != &timer_wait_list) {
		t_timer = rt_em_get_timer_by_timer_list(list);
		if (timeout >= t_timer->timeout)
			timeout -= t_timer->timeout;
		else {
			t_timer->timeout -= timeout;
			break;
		}
		list = list->next;
	}
	timer->timeout = timeout;
	rt_em_list_add_tail(list, &timer->timer_list);
	return 0;
}
//--------------------------------------------------------------
static int rt_em_timer_restart(em_timer_t* timer, unsigned timeout,
		enum em_timer_type type, void (*func)(void *), void *par)
{
	if (type == EM_TIMER_REUSE && timeout == 0)
		timeout = 1;
	timer->alltimeout = timeout;
	timer->type = type;
	timer->func = func;
	timer->par = par;
	__del_timer_wait_list(timer);
	return __add_timer_wait_list(timer, timeout);
}
//--------------------------------------------------------------
// Poll wait list. Need to call into ISR every 1 ms.
//--------------------------------------------------------------
void rt_timer_poll_wait_list(void)
{
	em_timer_t* timer;

	if (rt_em_is_list_empty(&timer_wait_list) == 0) {
		timer = rt_em_get_timer_by_timer_list(timer_wait_list.next);
		if (timer->timeout == 0 || --timer->timeout == 0) {
			// finish waiting this timer
			rt_em_list_remove_entry(&timer->timer_list);
			rt_em_list_add_tail(&timer_ready_list, &timer->timer_list);
			// finish waiting other timers with timeout == 0
			while (rt_em_is_list_empty(&timer_wait_list) == 0) {
				timer = rt_em_get_timer_by_timer_list(timer_wait_list.next);
				if (timer->timeout)
					break;
				rt_em_list_remove_entry(&timer->timer_list);
				rt_em_list_add_tail(&timer_ready_list, &timer->timer_list);
			}
			rt_em_event_set(timer_event, EM_TIMER_FLAG);
		}
	}
}
//--------------------------------------------------------------
// Task Timer
//--------------------------------------------------------------
static void FuncTimerTask(void* par)
{
	em_timer_t* timer;
	void (*func)(void *);

	while (1) {
		em_event_wait(timer_event, EM_EVENT_MODE_OR | EM_EVENT_MODE_CLEAR, EM_TIMER_FLAG, EM_TIM_INFINITY, NULL);
		do {
			timer = NULL;
			func = NULL;

			EM_DISABLE_TASK();

			if (rt_em_is_list_empty(&timer_ready_list) == 0) {
				timer = rt_em_get_timer_by_timer_list(timer_ready_list.next);
				rt_em_list_remove_entry(&timer->timer_list);
				func = timer->func;
				par = timer->par;
				if (timer->type == EM_TIMER_REUSE)
					rt_em_timer_restart(timer, timer->alltimeout, timer->type,
							timer->func, timer->par);
			}

			EM_ENABLE_TASK();

			if (func)
				func(par);
		} while (timer);
	}
}
//--------------------------------------------------------------
// Initialization module
//--------------------------------------------------------------
void em_timer_module_init(void)
{
	em_event_init(timer_event);

#if (cfgSTATIC_TIMER_TASK)
	C_em_task *task;
	unsigned *sp_start;
	unsigned sp_size;

	extern void em_get_timer_task(em_task_t **pptask, unsigned **ppsp_buf, unsigned * ppsp_size);
	em_get_timer_task(&task, &sp_start, &sp_size);
	em_task_init(task, "TimerTask", cfgPRI_TIMER_TASK, sp_start, sp_size, NULL, FuncTimerTask, 1);
#else
	em_task_new("TimerTask", cfgPRI_TIMER_TASK, cfgSIZE_STACK_TIMER_TASK, NULL, FuncTimerTask, 1);
#endif
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_timer_init(em_timer_t* timer)
{
	EM_ASSERT(timer != NULL);

	memset(timer, 0, sizeof(em_timer_t));
	rt_em_list_clear(&timer->timer_list);
}
//--------------------------------------------------------------
// Start
//--------------------------------------------------------------
void em_timer_start(em_timer_t* timer, unsigned timeout,
		enum em_timer_type type, void (*func)(void *), void *par)
{
	EM_ASSERT(timer != NULL);

	EM_DISABLE_TASK();

	int rc = rt_em_timer_restart(timer, timeout, type, func, par);

	EM_ENABLE_TASK();

	if (rc)
		em_event_set(timer_event, EM_TIMER_FLAG);
}
//--------------------------------------------------------------
// Restart
//--------------------------------------------------------------
void em_timer_restart(em_timer_t* timer)
{
	EM_ASSERT(timer != NULL);

	EM_DISABLE_TASK();

	int rc = rt_em_timer_restart(timer, timer->alltimeout, timer->type, timer->func, timer->par);

	EM_ENABLE_TASK();

	if (rc)
		em_event_set(timer_event, EM_TIMER_FLAG);
}
//--------------------------------------------------------------
// Stop
//--------------------------------------------------------------
void em_timer_stop(em_timer_t* timer)
{
	EM_ASSERT(timer != NULL);

	EM_DISABLE_TASK();

	__del_timer_wait_list(timer);

	EM_ENABLE_TASK();
}

#endif
