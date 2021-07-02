//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_TASK_H_
#define _EM_TASK_H_

// State in task
enum
{
	_S_INIT,
	_S_SUSPEND,
	_S_REDY,
	_S_DELETING,
	_S_WAIT,
	_S_WAIT_EVENT,
	_S_WAIT_SEM,
	_S_WAIT_MUTEX,
	_S_WAIT_QUEUE_POST,
	_S_WAIT_QUEUE_FETCH,
};

// Error
enum {
	EMERR_OK = 0,
	EMERR_TIMEOUT = -1,
	EMERR_DEINIT = -2,
	EMERR_OTHER_ERR = -3,
};

typedef uint32_t em_systick_t;
//--------------------------------------------------------------
// Scheduler
//--------------------------------------------------------------
void em_kernel_init(void);
void em_kernel_start(void);
void em_scheduler_disable(void);
void em_scheduler_enable(void);
//--------------------------------------------------------------
// Task UCB
//--------------------------------------------------------------
typedef struct _em_task_t
{
	unsigned* sp; 				// Pointer to SP
	unsigned* sp_start; 		// Start SP
	unsigned sp_size; 			// Size SP
	void* par; 					// Pointer to parameter
	void (*start_func) (void*); // Point of entry
	em_list_t init_list;
	em_list_t task_list; 		// Task List
	em_list_t mutex_lock_list; 	// Mutex Lock List
	em_list_t wait_list;		// Waiting List
	unsigned timeout; 			// Timeout
	int error; 					// Error return
	char name[cfgLEN_TASK_NAME]; // Task Name
	struct
	{
		unsigned pri :8; 		// Priority
		unsigned base_pri :8;	// Base priority
		unsigned state :8;		// State
		unsigned rezerv :8;
	};
	union
	{
		// For events
		struct
		{
			unsigned flags;	// Flags
			unsigned mode; 	// Mode
		} event;
		// For queue
		struct
		{
			void* msg;		// Message
			unsigned mode;	// Mode
		} msg;
	};
	void* wait_object;		// Waiting object (mutex, event ...)
} em_task_t;

void em_task_start(em_task_t* task);
void em_task_init(em_task_t* task,          // Pointer to task UCB
		const char* name,                   // Name
		unsigned pri,                       // Priority
		unsigned* sp_start,                 // Start SP
		unsigned sp_size,                   // Size SP
		void* par,                          // Pointer to parameter
		void (*start_func)(void*),          // Point of entry
		int fstart                          // Starting flag
		);
em_task_t* em_task_new(const char* name,	// Name
		unsigned pri,                       // Priority
		unsigned sp_size,                   // Size SP
		void* par,                          // Pointer to parameter
		void (*start_func)(void*),          // Point of entry
		int fstart                          // Starting flag
		);
unsigned em_task_get_pri(em_task_t* task);
void em_task_set_pri(em_task_t* task, unsigned pri);
void em_test_sp(void);
void em_yield(void);
void em_delay(unsigned timeout);
//--------------------------------------------------------------
// Systems Tick
// These functions can be called both in tasks and in ISR
//--------------------------------------------------------------
// msec
em_systick_t GetSysTic(void);
em_systick_t SubSysTic(em_systick_t tick);
// sec
em_systick_t GetSysTicSec(void);
em_systick_t SubSysTicSec(em_systick_t tick);
//--------------------------------------------------------------
// Errno
//--------------------------------------------------------------
int *em_errno(void);
#define errno *em_errno()
//--------------------------------------------------------------
// Utils
//--------------------------------------------------------------
unsigned em_strlcpy(char *s1, const char *s2, unsigned size);

#ifdef _EM_TASK_PRIVATE_
#ifndef _EM_TASK_
#define _EM_TASK_
	extern em_task_t *ctask;            // Current task
#endif /* _EM_TASK_ */

static inline em_task_t* rt_em_get_task_by_task_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_task_t, task_list);
}
void rt_em_add_wait_action(em_task_t* task, void* wait_object, em_list_t *list, unsigned state, unsigned timeout);
void rt_em_complete_wait_action(em_task_t* task, int error);
void rt_em_task_set_pri(em_task_t* task, unsigned pri);
void rt_em_scheduler();
int rt_em_scheduler_is_enable(void);
void rt_em_switch_context(void);
void rt_em_isr_system_tic(void);

#endif /* _EM_TASK_PRIVATE_ */

#endif /* _EM_TASK_H_ */

