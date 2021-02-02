//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

static __EM_LIST_INIT(mutex_init_list);

//--------------------------------------------------------------
// The priority of the mutex is determined by the priority of the tasks waiting for it
//--------------------------------------------------------------
static em_task_t* __synhr_pri_mutex(em_mutex_t* mutex)
{
	em_list_t* lst;
	em_task_t* task;
	unsigned old_pri;
	unsigned pri = 0;

	lst = mutex->wait_list.next;
	while (lst != &mutex->wait_list) {
		task = rt_em_get_task_by_task_list(lst);
		lst = lst->next;
		if (pri < task->pri)
			pri = task->pri;
	}

	if (mutex->pri != pri) {
		old_pri = mutex->pri;
		mutex->pri = pri;
		if (mutex->ptask && mutex->ptask->pri == old_pri)
			return mutex->ptask;
	}

	return NULL;
}
//--------------------------------------------------------------
// The priority of the task is determined by the priority of the locked mutexes
//--------------------------------------------------------------
static em_mutex_t* __synhr_pri_task(em_task_t* task)
{
	em_list_t* lst;
	em_mutex_t* mutex;
	unsigned old_pri;
	unsigned pri = task->base_pri;

	lst = task->mutex_lock_list.next;
	while (lst != &task->mutex_lock_list) {
		mutex = rt_em_get_mutex_by_lock_list(lst);
		lst = lst->next;
		if (pri < mutex->pri) {
			pri = mutex->pri;
		}
	}

	if (task->pri != pri) {
		old_pri = task->pri;
		rt_em_task_set_pri(task, pri);
		if (task->state == _S_WAIT_MUTEX
				&& ((em_mutex_t*) task->wait_object)->pri == old_pri)
			return task->wait_object;
	}

	return NULL;
}
//--------------------------------------------------------------
// Mutex priority synchronization
//--------------------------------------------------------------
void rt_em_synhr_pri_mutex(em_mutex_t* mutex)
{
	em_task_t* task;
	while ((task = __synhr_pri_mutex(mutex)) &&
			(mutex = __synhr_pri_task(task)));
}
//--------------------------------------------------------------
// Task priority synchronization
//--------------------------------------------------------------
void rt_em_synhr_pri_task(em_task_t* task)
{
	em_mutex_t* mutex;
	while ((mutex = __synhr_pri_task(task)) &&
			(task = __synhr_pri_mutex(mutex)));
}
//--------------------------------------------------------------
// Get mutex initialization list
//--------------------------------------------------------------
em_list_t* rt_em_get_mutex_init_list(void)
{
	return &mutex_init_list;
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_mutex_init(em_mutex_t* mutex)
{
	EM_ASSERT(mutex != NULL);

	EM_DISABLE_TASK();

	mutex->ptask = NULL;
	mutex->coun = 0;
	mutex->pri = 0;
	rt_em_list_clear(&mutex->wait_list);
	rt_em_list_clear(&mutex->lock_list);
	rt_em_list_add_tail(&mutex_init_list, &mutex->init_list);

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// De-initialization
//--------------------------------------------------------------
void em_mutex_deinit(em_mutex_t* mutex)
{
	em_task_t* task;

	EM_ASSERT(mutex != NULL);

	EM_DISABLE_TASK();

	while (rt_em_is_list_empty(&mutex->wait_list) == 0) {
		task = rt_em_get_task_by_task_list(mutex->wait_list.next);
		rt_em_complete_wait_action(task, EMERR_DEINIT);
	}
	if (mutex->ptask) {
		rt_em_list_remove_entry(&mutex->lock_list);
		rt_em_synhr_pri_task(mutex->ptask);
		mutex->ptask = NULL;
	}
	rt_em_list_remove_entry(&mutex->init_list);
	rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Create
//--------------------------------------------------------------
em_mutex_t* em_mutex_new(void)
{
	em_mutex_t* mutex = em_malloc(sizeof(em_mutex_t));
	if (mutex == NULL)
		return NULL;

	em_mutex_init(mutex);

	return mutex;
}
//--------------------------------------------------------------
// Delete
//--------------------------------------------------------------
void em_mutex_delete(em_mutex_t* mutex)
{
	EM_ASSERT(mutex != NULL);

	em_mutex_deinit(mutex);
	em_free(mutex);
}
//--------------------------------------------------------------
// Take
//--------------------------------------------------------------
int em_mutex_take(em_mutex_t* mutex, unsigned timeout)
{
	EM_ASSERT(mutex != NULL);

	if (ctask == NULL)
		return EMERR_OK;

	EM_DISABLE_TASK();

	if (mutex->coun == 0) {
		mutex->ptask = ctask;
		mutex->coun++;
		rt_em_list_add_tail(&ctask->mutex_lock_list, &mutex->lock_list);
		rt_em_synhr_pri_task(mutex->ptask);
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (mutex->ptask == ctask) {
		if (mutex->coun != EM_MAX_UNSIGNED_VALUE)
			mutex->coun++;
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (timeout == 0) {
		EM_ENABLE_TASK();
		return EMERR_TIMEOUT;
	}

	EM_ASSERT(rt_em_scheduler_is_enable());

	ctask->error = EMERR_TIMEOUT;
	rt_em_add_wait_action(ctask, mutex, &mutex->wait_list, _S_WAIT_MUTEX,
			timeout);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	return ctask->error;
}
//--------------------------------------------------------------
// Give
//--------------------------------------------------------------
int em_mutex_give_ex(em_mutex_t* mutex, int f_all)
{
	em_task_t *task;

	EM_ASSERT(mutex != NULL);

	if (ctask == NULL)
		return EMERR_OK;

	EM_DISABLE_TASK();

	if (mutex->coun == 0 || mutex->ptask != ctask) {
		EM_ENABLE_TASK();
		return EMERR_OTHER_ERR;
	}

	if (f_all)
		mutex->coun = 0;
	else if (mutex->coun)
		mutex->coun--;
	if (mutex->coun == 0) {
		rt_em_list_remove_entry(&mutex->lock_list);
		rt_em_synhr_pri_task(mutex->ptask);
		mutex->ptask = NULL;
		// Checking tasks in list
		if (rt_em_is_list_empty(&mutex->wait_list) == 0) {
			task = rt_em_get_task_by_task_list(mutex->wait_list.next);
			rt_em_complete_wait_action(task, EMERR_OK);
			// Now mutex belongs to this task
			mutex->ptask = task;
			mutex->coun++;
			rt_em_list_add_tail(&task->mutex_lock_list, &mutex->lock_list);
			rt_em_synhr_pri_task(mutex->ptask);
		}
		rt_em_scheduler();
	}

	EM_ENABLE_TASK();

	return EMERR_OK;
}
