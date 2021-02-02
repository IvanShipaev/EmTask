//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

static __EM_LIST_INIT(event_init_list);

//--------------------------------------------------------------
static inline void rt_em_event_clean(em_event_t* ev, unsigned flags)
{
	ev->flags &= ~flags;
}
//--------------------------------------------------------------
static unsigned rt_em_event_is_set(em_event_t* ev, enum ev_mode mode,
		unsigned flags)
{
	unsigned ev_flag = ev->flags;

	if (mode & EM_EVENT_MODE_AND) {
		if ((ev_flag & flags) == flags)
			return flags;
		return 0;
	}

	return ev_flag & flags;
}
//--------------------------------------------------------------
int rt_em_event_set(em_event_t* ev, unsigned flags)
{
	em_list_t *lst;
	em_task_t *task;
	unsigned rc;
	unsigned fclean = 0;

	if ((ev->flags & flags) != flags) {
		ev->flags |= flags;
		// Checking tasks in list
		lst = ev->wait_list.next;
		while (lst != &ev->wait_list) {
			task = rt_em_get_task_by_task_list(lst);
			lst = lst->next;
			rc = rt_em_event_is_set(ev, task->event.mode, task->event.flags);
			if (rc) {
				if (task->event.mode & EM_EVENT_MODE_CLEAR)
					fclean |= rc;
				task->event.flags = rc;
				rt_em_complete_wait_action(task, EMERR_OK);
			}
		}
		if (fclean)
			rt_em_event_clean(ev, fclean);
		rt_em_scheduler();
	}

	return EMERR_OK;
}
//--------------------------------------------------------------
// Get event initialization list
//--------------------------------------------------------------
em_list_t* rt_em_get_event_init_list(void)
{
	return &event_init_list;
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_event_init(em_event_t* ev)
{
	EM_ASSERT(ev != NULL);

	EM_DISABLE_TASK();

	ev->flags = 0;
	rt_em_list_clear(&ev->wait_list);
	rt_em_list_add_tail(&event_init_list, &ev->init_list);

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// De-initialization
//--------------------------------------------------------------
void em_event_deinit(em_event_t* ev)
{
	em_task_t* task;

	EM_ASSERT(ev != NULL);

	EM_DISABLE_TASK();

	while (rt_em_is_list_empty(&ev->wait_list) == 0) {
		task = rt_em_get_task_by_task_list(ev->wait_list.next);
		rt_em_complete_wait_action(task, EMERR_DEINIT);
	}
	rt_em_list_remove_entry(&ev->init_list);
	rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Create
//--------------------------------------------------------------
em_event_t* em_event_new(void)
{
	em_event_t* ev = em_malloc(sizeof(em_event_t));

	if (ev)
		em_event_init(ev);

	return ev;
}
//--------------------------------------------------------------
// Delete
//--------------------------------------------------------------
void em_event_delete(em_event_t* ev)
{
	EM_ASSERT(ev != NULL);

	em_event_deinit(ev);
	em_free(ev);
}
//--------------------------------------------------------------
// Test
//--------------------------------------------------------------
unsigned em_event_is_set(em_event_t* ev, enum ev_mode mode, unsigned flags)
{
	unsigned rc;

	EM_DISABLE_TASK();

	rc = rt_em_event_is_set(ev, mode, flags);

	EM_ENABLE_TASK();

	return rc;
}
//--------------------------------------------------------------
// Wait
//--------------------------------------------------------------
int em_event_wait(em_event_t* ev, enum ev_mode mode, unsigned flags,
		unsigned timeout, unsigned* value)
{
	unsigned rc;

	EM_DISABLE_TASK();

	if ((mode & EM_EVENT_MODE_AND) == 0)
		mode |= EM_EVENT_MODE_OR;

	rc = rt_em_event_is_set(ev, mode, flags);
	if (rc) {
		if (mode & EM_EVENT_MODE_CLEAR)
			rt_em_event_clean(ev, rc);
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (timeout == 0) {
		EM_ENABLE_TASK();
		return EMERR_TIMEOUT;
	}

	EM_ASSERT(rt_em_scheduler_is_enable());

	// TASK
	ctask->event.mode = mode;
	ctask->event.flags = flags;
	ctask->error = EMERR_TIMEOUT;
	rt_em_add_wait_action(ctask, ev, &ev->wait_list, _S_WAIT_EVENT, timeout);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	if (ctask->error == EMERR_OK && value)
		*value = ctask->event.flags;

	return ctask->error;
}
//--------------------------------------------------------------
// Set
//--------------------------------------------------------------
int em_event_set(em_event_t* ev, unsigned flags)
{
	int rc;

	EM_ASSERT(ev != NULL);

	EM_DISABLE_TASK();

	rc = rt_em_event_set(ev, flags);

	EM_ENABLE_TASK();

	return rc;
}
//--------------------------------------------------------------
// Clean
//--------------------------------------------------------------
int em_event_clean(em_event_t* ev, unsigned flags)
{
	EM_ASSERT(ev != NULL);

	EM_DISABLE_TASK();

	rt_em_event_clean(ev, flags);

	EM_ENABLE_TASK();

	return EMERR_OK;
}
//--------------------------------------------------------------
//	Sync
//--------------------------------------------------------------
int em_event_sync_flags(em_event_t* ev, unsigned flags)
{
	unsigned t_flags;
	int rc;

	EM_ASSERT(ev != NULL);

	EM_DISABLE_TASK();

	t_flags = (ev->flags & flags);
	rt_em_event_clean(ev, t_flags ^ ev->flags);
	rc = rt_em_event_set(ev, t_flags ^ flags);

	EM_ENABLE_TASK();

	return rc;
}
