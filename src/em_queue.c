//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

static __EM_LIST_INIT(queue_init_list);

//---------------------------------------------------------------
static int __do_post_msg(em_queue_t *queue, void *msg, enum queue_mode mode)
{
	if (queue->num_block >= queue->max_block)
		return 0;
	if (mode & QUEUE_MODE_TAIL) {
		memcpy(queue->buf + queue->tail, msg, queue->size_block);
		queue->tail += queue->size_block;
		if (queue->tail >= queue->len_buf)
			queue->tail = 0;
	} else {
		if (queue->head == 0)
			queue->head = queue->len_buf;
		queue->head -= queue->size_block;
		memcpy(queue->buf + queue->head, msg, queue->size_block);
	}
	queue->num_block++;
	return 1;
}
//---------------------------------------------------------------
static int __do_fetch_msg(em_queue_t *queue, void *msg, enum queue_mode mode)
{
	unsigned tail;
	if (queue->num_block == 0)
		return 0;
	if (mode & QUEUE_MODE_HEAD) {
		memcpy(msg, queue->buf + queue->head, queue->size_block);
		if ((mode & QUEUE_MODE_NO_DELETE) == 0) {
			queue->head += queue->size_block;
			if (queue->head >= queue->len_buf)
				queue->head = 0;
			queue->num_block--;
		}
	} else {
		tail = queue->tail;
		if (tail == 0)
			tail = queue->len_buf;
		tail -= queue->size_block;
		memcpy(msg, queue->buf + tail, queue->size_block);
		if ((mode & QUEUE_MODE_NO_DELETE) == 0) {
			queue->tail = tail;
			queue->num_block--;
		}
	}
	return 1;
}
//--------------------------------------------------------------
// Get queue initialization list
//--------------------------------------------------------------
em_list_t* rt_em_get_queue_init_list(void)
{
	return &queue_init_list;
}
//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_queue_init(em_queue_t* queue, unsigned size_block, unsigned nblock,
		unsigned char* buf)
{
	EM_ASSERT(queue != NULL);

	EM_DISABLE_TASK();

	queue->head = 0;
	queue->tail = 0;
	queue->num_block = 0;
	queue->size_block = size_block;
	queue->max_block = nblock;
	queue->len_buf = size_block * nblock;
	queue->buf = buf;
	rt_em_list_clear(&queue->wait_fetch_list);
	rt_em_list_clear(&queue->wait_post_list);
	rt_em_list_add_tail(&queue_init_list, &queue->init_list);

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// De-initialization
//--------------------------------------------------------------
void em_queue_deinit(em_queue_t* queue)
{
	em_task_t* task;

	EM_ASSERT(queue != NULL);

	EM_DISABLE_TASK();

	// Checking tasks in list
	while (rt_em_is_list_empty(&queue->wait_post_list) == 0) {
		task = rt_em_get_task_by_task_list(queue->wait_post_list.next);
		rt_em_complete_wait_action(task, EMERR_DEINIT);
	}
	// Checking tasks in list
	while (rt_em_is_list_empty(&queue->wait_fetch_list) == 0) {
		task = rt_em_get_task_by_task_list(queue->wait_fetch_list.next);
		rt_em_complete_wait_action(task, EMERR_DEINIT);
	}
	rt_em_list_remove_entry(&queue->init_list);
	rt_em_scheduler();

	EM_ENABLE_TASK();
}
//--------------------------------------------------------------
// Create
//--------------------------------------------------------------
em_queue_t* em_queue_new(unsigned size_block, unsigned max_block)
{
	unsigned char* buf;
	em_queue_t *queue = em_malloc(sizeof(em_queue_t));
	if (queue == NULL)
		return NULL;

	buf = em_malloc(size_block * max_block);
	if (buf == NULL) {
		em_free(queue);
		return NULL;
	}

	em_queue_init(queue, size_block, max_block, buf);
	return queue;
}
//--------------------------------------------------------------
// Delete
//--------------------------------------------------------------
void em_queue_delete(em_queue_t* queue)
{
	EM_ASSERT(queue != NULL);

	em_queue_deinit(queue);
	em_free(queue->buf);
	em_free(queue);
}
//---------------------------------------------------------------
// Post message
//---------------------------------------------------------------
int em_queue_post(em_queue_t *queue, void *msg, unsigned timeout,
		enum queue_mode mode)
{
	em_task_t *task;

	EM_ASSERT(queue != NULL);

	if (mode & QUEUE_MODE_HEAD)
		mode &= ~(QUEUE_MODE_TAIL);
	else
		mode |= QUEUE_MODE_TAIL;

	EM_DISABLE_TASK();

	if (__do_post_msg(queue, msg, mode)) {
		if (rt_em_is_list_empty(&queue->wait_fetch_list) == 0) {
			task = rt_em_get_task_by_task_list(queue->wait_fetch_list.next);
			__do_fetch_msg(queue, task->msg.msg, task->msg.mode);
			rt_em_complete_wait_action(task, EMERR_OK);
			rt_em_scheduler();
		}
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (timeout == 0) {
		EM_ENABLE_TASK();
		return EMERR_TIMEOUT;
	}

	EM_ASSERT(rt_em_scheduler_is_enable());

	// TASK
	ctask->msg.msg = msg;
	ctask->msg.mode = mode;
	ctask->error = EMERR_TIMEOUT;
	rt_em_add_wait_action(ctask, queue, &queue->wait_post_list,
			_S_WAIT_QUEUE_POST, timeout);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	return ctask->error;
}
//---------------------------------------------------------------
// Fetch message
//---------------------------------------------------------------
int em_queue_fetch(em_queue_t* queue, void* msg, unsigned timeout,
		enum queue_mode mode)
{
	EM_ASSERT(queue != NULL);

	em_task_t *task;

	if (mode & QUEUE_MODE_TAIL)
		mode &= ~(QUEUE_MODE_HEAD);
	else
		mode |= QUEUE_MODE_HEAD;

	EM_DISABLE_TASK();

	if (__do_fetch_msg(queue, msg, mode)) {
		if (rt_em_is_list_empty(&queue->wait_post_list) == 0) {
			task = rt_em_get_task_by_task_list(queue->wait_post_list.next);
			__do_post_msg(queue, task->msg.msg, task->msg.mode);
			rt_em_complete_wait_action(task, EMERR_OK);
			rt_em_scheduler();
		}
		EM_ENABLE_TASK();
		return EMERR_OK;
	} else if (timeout == 0) {
		EM_ENABLE_TASK();
		return EMERR_TIMEOUT;
	}

	EM_ASSERT(rt_em_scheduler_is_enable());

	// TASK
	ctask->msg.msg = msg;
	ctask->msg.mode = mode;
	ctask->error = EMERR_TIMEOUT;
	rt_em_add_wait_action(ctask, queue, &queue->wait_fetch_list,
			_S_WAIT_QUEUE_FETCH, timeout);
	rt_em_scheduler();

	EM_ENABLE_TASK();

	return ctask->error;
}
