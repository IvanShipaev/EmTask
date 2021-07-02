//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_
#define _EM_TASK_PRIVATE_
#include "task_include.h"
#define EM_YIELD_FLAG	(1 << 0)
#define EM_DELAY_FLAG	(1 << 1)
//--------------------------------------------------------------
static __EM_LIST_INIT(task_init_list);
static __EM_LIST_INIT(task_start_list);
static __EM_LIST_INIT(task_wait_list);
static __EM_LIST_INIT(task_deleting_list);
static em_list_t task_ready_list[cfgNUM_PRI];   // Array of priority queues of tasks ready for execution
static unsigned task_mask_pri = 0;              // Mask of priorities tasks ready for execution
static em_event_t ev_sys[1];                    // System event for yield() and delay()
static unsigned counter_scheduler_disable = 0x7fffffff;
static volatile struct                          // System timer
{
	em_systick_t ms;
	em_systick_t sec;
	em_systick_t min;
	em_systick_t hour;
} sys_timer;
em_task_t *ctask = NULL;                        // Pointer to current task
//--------------------------------------------------------------
inline em_task_t* rt_em_get_task_by_wait_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_task_t, wait_list);
}
//--------------------------------------------------------------
static void __delete_procedure(void)
{
	em_task_t* task = NULL;

	do {
		task = NULL;

		EM_DISABLE_TASK();

		if (rt_em_is_list_empty(&task_deleting_list) == 0) {
			task = rt_em_get_task_by_task_list(task_deleting_list.next);
			rt_em_list_remove_entry(&task->task_list);
			rt_em_list_remove_entry(&task->init_list);
		}

		EM_ENABLE_TASK();

		if (task) {
			em_free(task->sp_start);
			em_free(task);
		}
	} while (task);
}
//--------------------------------------------------------------
static void FuncIdleTask(void* par)
{
	(void) par;

	while (1) {
#if (cfgUSE_IDLE_HOOK==1)
		extern void em_idle_hook(void);
		em_idle_hook();
#endif /*cfgUSE_IDLE_HOOK*/
		__delete_procedure();
		em_event_set(ev_sys, EM_YIELD_FLAG);
		// Note: Add Sleeping ...
	}
}
//--------------------------------------------------------------
// Add task in waiting list
//--------------------------------------------------------------
static void __add_task_wait_list(em_task_t* task, unsigned timeout)
{
	em_list_t* list;
	em_task_t* t_task;

	list = task_wait_list.next;
	while (list != &task_wait_list) {
		t_task = rt_em_get_task_by_wait_list(list);
		if (timeout >= t_task->timeout)
			timeout -= t_task->timeout;
		else {
			t_task->timeout -= timeout;
			break;
		}
		list = list->next;
	}
	task->timeout = timeout;
	rt_em_list_add_tail(list, &task->wait_list);
}
//--------------------------------------------------------------
// Delete task from waiting list
//--------------------------------------------------------------
static void __del_task_wait_list(em_task_t* task)
{
	em_task_t* t_task;

	if (rt_em_is_list_empty(&task->wait_list) == 0) {
		if (task->wait_list.next != &task_wait_list) {
			t_task = rt_em_get_task_by_wait_list(task->wait_list.next);
			t_task->timeout += task->timeout;
		}
		rt_em_list_remove_entry(&task->wait_list);
	}
}
//--------------------------------------------------------------
// Add task in waiting action list with timeout
//--------------------------------------------------------------
void rt_em_add_wait_action(em_task_t* task, void* wait_object, em_list_t *list,
		unsigned state, unsigned timeout)
{
	__del_task_wait_list(task);
	if (timeout != EM_TIM_INFINITY)
		__add_task_wait_list(task, timeout);
	rt_em_list_remove_entry(&task->task_list);
	rt_em_list_add_tail(list, &task->task_list);
	task->state = state;
	task->wait_object = wait_object;
	if (rt_em_is_list_empty(&task_ready_list[task->pri]))
		task_mask_pri &= ~(1 << task->pri);
	if (state == _S_WAIT_MUTEX)
		rt_em_synhr_pri_mutex((em_mutex_t*)wait_object);
}
//--------------------------------------------------------------
// Complete waiting action
//--------------------------------------------------------------
void rt_em_complete_wait_action(em_task_t* task, int error)
{
	unsigned state = task->state;
	__del_task_wait_list(task);
	rt_em_list_remove_entry(&task->task_list);
	rt_em_list_add_tail(&task_ready_list[task->pri], &task->task_list);
	task->state = _S_REDY;
	task->error = error;
	task_mask_pri |= (1 << task->pri);
	if (state == _S_WAIT_MUTEX)
		rt_em_synhr_pri_mutex((em_mutex_t*)task->wait_object);
}
//-------------------------------------------------------------
// Switch Context
//-------------------------------------------------------------
void rt_em_switch_context(void)
{
	unsigned task_pri_redy;

#if cfgTEST_SP_SWITCH_TASK
	extern void rt_em_port_test_sp_switch_task(void);
	rt_em_port_test_sp_switch_task();
#endif /* cfgTEST_SP_SWITCH_TASK */

	task_pri_redy = rt_em_find_first_bit(task_mask_pri);
	ctask = rt_em_get_task_by_task_list(task_ready_list[task_pri_redy].next);
	rt_em_list_jump_head(&task_ready_list[task_pri_redy]);
}
//--------------------------------------------------------------
//--------------------------------------------------------------
// Scheduler
//--------------------------------------------------------------
//--------------------------------------------------------------
// Switch context if needed with round robin flag
//-------------------------------------------------------------
static void __scheduler_ex(int froundrobin)
{
	unsigned task_pri_redy;
	if (counter_scheduler_disable == 0) {
		task_pri_redy = rt_em_find_first_bit(task_mask_pri);
		if (task_pri_redy > ctask->pri)
			rt_em_port_request_switch_context();
		else if (ctask->state != _S_REDY)
			rt_em_port_request_switch_context();
		else if (froundrobin) {
			if (ctask->task_list.next != ctask->task_list.prev)
				rt_em_port_request_switch_context();
		}
	}
}
//--------------------------------------------------------------
// Switch context if needed without round robin
//--------------------------------------------------------------
void rt_em_scheduler(void)
{
	__scheduler_ex(0);
}
//--------------------------------------------------------------
// Is schedule enable
//--------------------------------------------------------------
int rt_em_scheduler_is_enable(void)
{
	return !counter_scheduler_disable;
}
//--------------------------------------------------------------
// Initialization kernel
//--------------------------------------------------------------
static void em_idle_task_init(void)
{
#if cfgSTATIC_SYSTEM_TASK
	em_task_t *task;
	unsigned *sp_start;
	unsigned sp_size;

	extern void em_get_idle_task(em_task_t **pptask, unsigned **ppsp_buf, unsigned * ppsp_size);
	em_get_idle_task(&task, &sp_start, &sp_size);
	em_task_init(task, "IdleTask", 0, sp_start, sp_size, NULL, Func_idle_task, 1);
	ctask = task;
#else
	ctask = em_task_new("IdleTask", 0, cfgSIZE_STACK_IDLE_TASK, NULL, FuncIdleTask, 1);
#endif
}
//--------------------------------------------------------------
void em_kernel_init(void)
{
	em_mem_init();

	for (int i = 0; i < cfgNUM_PRI; i++)
		rt_em_list_clear(&task_ready_list[i]);
	em_event_init(ev_sys);

	em_idle_task_init();

#if (cfgUSE_TIMER_TASK)
	em_timer_module_init();
#endif
}
//--------------------------------------------------------------
// Start kernel
//--------------------------------------------------------------
void em_kernel_start(void)
{
	counter_scheduler_disable = 0;
	rt_em_port_start_first_task();
}
//--------------------------------------------------------------
// Disable scheduler
//--------------------------------------------------------------
void em_scheduler_disable(void)
{
	EM_DISABLE_TASK();

	counter_scheduler_disable++;

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Enable scheduler
//--------------------------------------------------------------
void em_scheduler_enable(void)
{
	EM_DISABLE_TASK();

	if (counter_scheduler_disable)
		counter_scheduler_disable--;
	rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
//--------------------------------------------------------------
// TASK
//-------------------------------------------------------------
//--------------------------------------------------------------
// Function exit for task
//--------------------------------------------------------------
static void __em_task_exit(void)
{
	em_mutex_t* mutex;

	// Unlock mutexes
	do {
		mutex = NULL;

		EM_DISABLE_TASK();

		if (rt_em_is_list_empty(&ctask->mutex_lock_list) == 0)
			mutex = rt_em_get_mutex_by_lock_list((&ctask->mutex_lock_list)->next);

		EM_ENABLE_TASK();

		if (mutex)
			em_mutex_give_ex(mutex, 1);
	} while (mutex);

	EM_DISABLE_TASK();

	rt_em_add_wait_action(ctask, NULL, &task_deleting_list, _S_DELETING, EM_TIM_INFINITY);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	while (1);
}
//--------------------------------------------------------------
// Start task
//--------------------------------------------------------------
void em_task_start(em_task_t* task)
{
	EM_ASSERT(task != NULL);

	EM_DISABLE_TASK();

	if (task->state == _S_INIT) {
		rt_em_complete_wait_action(task, EMERR_OK);
		rt_em_scheduler();
	}

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_task_init(em_task_t* task,          // Pointer to task UCB
		const char* name,                   // Name
		unsigned pri,                       // Priority
		unsigned* sp_start,                 // Start SP
		unsigned sp_size,                   // Size SP
		void* par,                          // Pointer to parameter
		void (*start_func)(void*),          // Point of entry
		int fstart                          // Starting flag
		)
{
	EM_ASSERT(task != NULL);
	EM_ASSERT(sp_start != NULL);
	EM_ASSERT(start_func != NULL);

#if 1
	// NOTE: Can be deleted
	if (ctask == NULL)
		ctask = task;
#endif

	pri = EM_MIN(pri, (cfgNUM_PRI - 1));
	rt_em_list_clear(&task->task_list);
	rt_em_list_clear(&task->wait_list);
	rt_em_list_clear(&task->mutex_lock_list);
	em_strlcpy(task->name, name, cfgLEN_TASK_NAME);

	task->sp = rt_em_port_init_stack(sp_start, sp_size, start_func, par,
			__em_task_exit);
	task->sp_start = sp_start;
	task->sp_size = sp_size;
	task->par = par;
	task->start_func = start_func;
	task->error = EMERR_OK;
	task->pri = pri;
	task->base_pri = pri;
	task->state = _S_INIT;

	EM_DISABLE_TASK();

	rt_em_list_add_tail(&task_init_list, &task->init_list);
	rt_em_add_wait_action(task, NULL, &task_start_list, _S_INIT,
			EM_TIM_INFINITY);

	EM_ENABLE_TASK();

	if (fstart)
		em_task_start(task);
}
//--------------------------------------------------------------
// Create
//--------------------------------------------------------------
em_task_t* em_task_new(const char* name,	// Name
		unsigned pri,                       // Priority
		unsigned sp_size,                   // Size SP
		void* par,                          // Pointer to parameter
		void (*start_func)(void*),          // Point of entry
		int fstart                          // Starting flag
		)
{
	unsigned* sp_start;
	em_task_t* task = em_malloc(sizeof(em_task_t));

	if (task == NULL)
		return NULL;
	sp_start = em_malloc(sp_size * sizeof(unsigned));
	if (sp_start == NULL) {
		em_free(task);
		return NULL;
	}

	em_task_init(task, name, pri, sp_start, sp_size, par, start_func, fstart);
	return task;
}
//--------------------------------------------------------------
// Get priority
//--------------------------------------------------------------
unsigned em_task_get_pri(em_task_t* task)
{
	unsigned pri;

	EM_ASSERT(task != NULL);

	EM_DISABLE_TASK();

	pri = task->pri;

	EM_ENABLE_TASK();

	return pri;
}
//--------------------------------------------------------------
// Set priority
//--------------------------------------------------------------
void rt_em_task_set_pri(em_task_t* task, unsigned pri)
{
	if (task->state == _S_REDY) {
		rt_em_list_remove_entry(&task->task_list);
		if (rt_em_is_list_empty(&task_ready_list[task->pri]))
			task_mask_pri &= ~(1 << task->pri);
		task->pri = pri;
		rt_em_list_add_tail(&task_ready_list[task->pri], &task->task_list);
		task_mask_pri |= (1 << task->pri);
	}
	else
		task->pri = pri;
}
void em_task_set_pri(em_task_t* task, unsigned pri)
{
	EM_ASSERT(task != NULL);
	EM_ASSERT(ctask != NULL);

	EM_DISABLE_TASK();

	rt_em_task_set_pri(task, pri);
	task->base_pri = pri;
	if (ctask != task && task->pri > ctask->pri)
		rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Testing SP
//--------------------------------------------------------------
void em_test_sp(void)
{
	em_scheduler_disable();

	rt_em_port_test_sp(&task_init_list);

	em_scheduler_enable();
}
//--------------------------------------------------------------
// Yield
//--------------------------------------------------------------
void em_yield(void)
{
	em_event_wait(ev_sys, EM_EVENT_MODE_OR | EM_EVENT_MODE_CLEAR,
			EM_YIELD_FLAG, 1, NULL);
}
//--------------------------------------------------------------
// Delay
//--------------------------------------------------------------
void em_delay(unsigned timeout)
{
	em_event_wait(ev_sys, EM_EVENT_MODE_OR | EM_EVENT_MODE_CLEAR,
			EM_DELAY_FLAG, timeout, NULL);
}
//--------------------------------------------------------------
//--------------------------------------------------------------
// Tick
//--------------------------------------------------------------
//--------------------------------------------------------------
static void __poll_task_wait_list(void)
{
	em_task_t * task;

	if (rt_em_is_list_empty(&task_wait_list) == 0) {
		task = rt_em_get_task_by_wait_list(task_wait_list.next);
		if (task->timeout == 0 || --task->timeout == 0) {
			// finish waiting this task
			rt_em_complete_wait_action(task, EMERR_TIMEOUT);
			// finish waiting other tasks with timeout == 0
			while (rt_em_is_list_empty(&task_wait_list) == 0) {
				task = rt_em_get_task_by_wait_list(task_wait_list.next);
				if (task->timeout)
					break;
				rt_em_complete_wait_action(task, EMERR_TIMEOUT);
			}
		}
	}
	__scheduler_ex(1);
}

//--------------------------------------------------------------
static void __sys_tic(void)
{
	static uint16_t count_ms = 0;
	static uint8_t count_sec = 0;
	static uint8_t count_min = 0;
	sys_timer.ms++;
	if (++count_ms >= 1000) { // sec
		count_ms = 0;
		sys_timer.sec++;
		if (++count_sec >= 60) { // min
			count_sec = 0;
			sys_timer.min++;
			if (++count_min >= 24) { // hour
				count_min = 0;
				sys_timer.hour++;
			}
		}
	}
}
//--------------------------------------------------------------
void rt_em_isr_system_tic(void)
{
	EM_DISABLE_TASK();

	__sys_tic();
	__poll_task_wait_list();
#if (cfgUSE_TIMER_TASK)
	rt_timer_poll_wait_list();
#endif /* cfgUSE_TIMER_TASK */

#if (cfgUSE_SYSTIC_HOOK)
	extern void EM_SYS_TIC_HOOK(void);
	EM_SYS_TIC_HOOK();
#endif /* cfgUSE_SYSTIC_HOOK */

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
em_systick_t GetSysTic(void)
{
	return sys_timer.ms;
}
//--------------------------------------------------------------
em_systick_t SubSysTic(em_systick_t tick)
{
	return GetSysTic() - tick;
}
//--------------------------------------------------------------
// Sec
//--------------------------------------------------------------
em_systick_t GetSysTicSec(void)
{
	return sys_timer.sec;
}
//--------------------------------------------------------------
em_systick_t SubSysTicSec(em_systick_t tick)
{
	return GetSysTicSec() - tick;
}
//--------------------------------------------------------------
// Errno
//--------------------------------------------------------------
int *em_errno(void)
{
	return &ctask->error;
}
//--------------------------------------------------------------
// Utils
//--------------------------------------------------------------
unsigned em_strlcpy(char *s1, const char *s2, unsigned size)
{
	unsigned i;
	if (size == 0)
		return 0;
	for (i = 0; (i < size - 1) && (*s2 != 0); i++)
		*s1++ = *s2++;
	*s1 = 0;
	return i;
}
//--------------------------------------------------------------
// Assert
//--------------------------------------------------------------
#if (cfgUSE_ASSERT)
void __em_assert_func(char* file, unsigned line)
{
	EM_PRINTF("assert: %s - %d\r\n", file, line);
	EM_DISABLE_TASK();
	EM_DISABLE_ISR();
	while (1);
}
#endif
