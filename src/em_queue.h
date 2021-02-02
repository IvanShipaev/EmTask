//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_QUEUE_H_
#define _EM_QUEUE_H_

enum queue_mode
{
	QUEUE_MODE_DEFAULT = 0,				// Post in tail, fetch in head
	QUEUE_MODE_HEAD = (1 << 0),			// Post/fetch in head
	QUEUE_MODE_TAIL = (1 << 1), 		// Post/fetch in tail
	QUEUE_MODE_NO_DELETE = (1 << 2),	// Fetching without deleting
};

typedef struct _em_queue_t
{
	em_list_t init_list;
	em_list_t wait_fetch_list;
	em_list_t wait_post_list;
	unsigned head;
	unsigned tail;
	unsigned num_block;
	unsigned size_block;
	unsigned max_block;
	unsigned len_buf;
	unsigned char* buf;
} em_queue_t;

void em_queue_init(em_queue_t* queue, unsigned size_block, unsigned nblock,
		unsigned char* buf);
void em_queue_deinit(em_queue_t* queue);
em_queue_t* em_queue_new(unsigned size_block, unsigned nblock);
void em_queue_delete(em_queue_t *queue);
int em_queue_post(em_queue_t *queue, void *msg, unsigned timeout,
		enum queue_mode mode);
int em_queue_fetch(em_queue_t* queue, void* msg, unsigned timeout,
		enum queue_mode mode);
//--------------------------------------------------------------
// These functions can be called both in tasks and in ISR
//--------------------------------------------------------------
inline int em_queue_try_post(em_queue_t *queue, void *msg, enum queue_mode mode)
{
	return em_queue_post(queue, msg, 0, mode);
}
inline int em_queue_try_fetch(em_queue_t* queue, void* msg, enum queue_mode mode)
{
	return em_queue_fetch(queue, msg, 0, mode);
}

#ifdef _EM_TASK_PRIVATE_
em_list_t* rt_em_get_queue_init_list(void);
inline em_queue_t* rt_em_get_queue_by_init_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_queue_t, init_list);
}
#endif /* _EM_TASK_PRIVATE_ */

#endif /* _EM_QUEUE_H_ */
