//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_EVENT_H_
#define _EM_EVENT_H_

enum ev_mode
{
	EM_EVENT_MODE_OR = (1 << 0),		// Test by OR
	EM_EVENT_MODE_AND = (1 << 1),		// Test by AND
	EM_EVENT_MODE_CLEAR = (1 << 2),		// Clearing after waiting
};

typedef struct _em_event_t
{
	em_list_t init_list;
	em_list_t wait_list;	// Waiting tasks list
	unsigned flags;			// Flags
} em_event_t;

void em_event_init(em_event_t* ev);
void em_event_deinit(em_event_t* ev);
em_event_t* em_event_new(void);
void em_event_delete(em_event_t* ev);
int em_event_wait(em_event_t* ev, enum ev_mode mode, unsigned flags,
		unsigned timeout, unsigned *value);
//--------------------------------------------------------------
// These functions can be called both in tasks and in ISR
//--------------------------------------------------------------
int em_event_set(em_event_t* ev, unsigned flags);
int em_event_clean(em_event_t* ev, unsigned flags);
int em_event_sync_flags(em_event_t* ev, unsigned flags);
unsigned em_event_is_set(em_event_t* ev, enum ev_mode mode, unsigned flags);

#ifdef _EM_TASK_PRIVATE_
em_list_t* rt_em_get_event_init_list(void);
inline em_event_t* rt_em_get_event_by_init_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_event_t, init_list);
}
int rt_em_event_set(em_event_t* ev, unsigned flags);
#endif /* _EM_TASK_PRIVATE_ */

#endif /* _EM_EVENT_H_ */
