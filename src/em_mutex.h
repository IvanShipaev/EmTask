//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_MUTEX_H_
#define _EM_MUTEX_H_

typedef struct _em_mutex_t
{
	em_list_t init_list;
	em_list_t wait_list;
	em_list_t lock_list;
	em_task_t* ptask;
	unsigned coun;
	unsigned pri;
} em_mutex_t;


void em_mutex_init(em_mutex_t* mutex);
void em_mutex_deinit(em_mutex_t* mutex);
em_mutex_t* em_mutex_new(void);
void em_mutex_delete(em_mutex_t* mutex);
int em_mutex_take(em_mutex_t* mutex, unsigned timeout);
int em_mutex_give_ex(em_mutex_t* mutex, int f_all);
static inline int em_mutex_give(em_mutex_t* mutex)
{
	return em_mutex_give_ex(mutex, 0);
}

#ifdef _EM_TASK_PRIVATE_
em_list_t* rt_em_get_mutex_init_list(void);
static inline em_mutex_t* rt_em_get_mutex_by_init_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_mutex_t, init_list);
}
static inline em_mutex_t* rt_em_get_mutex_by_lock_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_mutex_t, lock_list);
}
void rt_em_synhr_pri_mutex(em_mutex_t* mutex);
#endif /* _EM_TASK_PRIVATE_ */

#endif /* _EM_MUTEX_H_ */
