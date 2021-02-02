//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

static __EM_LIST_INIT(sem_init_list);

//--------------------------------------------------------------
// Get semaphore initialization list
//--------------------------------------------------------------
em_list_t* rt_em_get_sem_init_list(void)
{
	return &sem_init_list;
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_sem_init(em_sem_t* sem, unsigned startcount, unsigned maxcount)
{
	EM_ASSERT(sem != NULL);
	EM_ASSERT(startcount <= maxcount);

	EM_DISABLE_TASK();

	sem->count = startcount;
	sem->maxcount = maxcount;
	rt_em_list_clear(&sem->wait_list);
	rt_em_list_add_tail(&sem_init_list, &sem->init_list);

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// De-initialization
//--------------------------------------------------------------
void em_sem_deinit(em_sem_t* sem)
{
	em_task_t* task;

	EM_ASSERT(sem != NULL);

	EM_DISABLE_TASK();

	// Checking tasks in list
	while (rt_em_is_list_empty(&sem->wait_list) == 0) {
		task = rt_em_get_task_by_task_list(sem->wait_list.next);
		rt_em_complete_wait_action(task, EMERR_DEINIT);
	}
	rt_em_list_remove_entry(&sem->init_list);
	rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Create
//--------------------------------------------------------------
em_sem_t* em_sem_new(unsigned startcount, unsigned maxcount)
{
	em_sem_t* sem;

	sem = em_malloc(sizeof(em_sem_t));

	if (sem)
		em_sem_init(sem, startcount, maxcount);

	return sem;
}
//--------------------------------------------------------------
// Delete
//--------------------------------------------------------------
void em_sem_delete(em_sem_t* sem)
{
	EM_ASSERT(sem != NULL);

	em_sem_deinit(sem);
	em_free(sem);
}
//--------------------------------------------------------------
// Take
//--------------------------------------------------------------
int em_sem_take(em_sem_t* sem, unsigned timeout)
{
	EM_ASSERT(sem != NULL);

	EM_DISABLE_TASK();

	if (sem->count) {
		sem->count--;
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (timeout == 0) {
		EM_ENABLE_TASK();
		return EMERR_TIMEOUT;
	}

	EM_ASSERT(rt_em_scheduler_is_enable());

	// TASK
	ctask->error = EMERR_TIMEOUT;
	rt_em_add_wait_action(ctask, sem, &sem->wait_list, _S_WAIT_SEM, timeout);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	return ctask->error;
}
//--------------------------------------------------------------
// Give
//--------------------------------------------------------------
int em_sem_give(em_sem_t* sem)
{
	EM_ASSERT(sem != NULL);

	em_task_t *task;

	EM_DISABLE_TASK();

	if (sem->count < sem->maxcount) {
		if (rt_em_is_list_empty(&sem->wait_list))
			sem->count++;
		else {
			task = rt_em_get_task_by_task_list(sem->wait_list.next);
			rt_em_complete_wait_action(task, EMERR_OK);
			rt_em_scheduler();
		}
	}

	EM_ENABLE_TASK();

	return EMERR_OK;
}
