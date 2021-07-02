//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_SEM_H_
#define _EM_SEM_H_

typedef struct _em_sem_t
{
	em_list_t init_list;
	em_list_t wait_list; 	// Waiting tasks list
	unsigned count; 		// Current count
	unsigned maxcount; 		// Max count
} em_sem_t;

void em_sem_init(em_sem_t* sem, unsigned startcount, unsigned maxcount);
void em_sem_deinit(em_sem_t* sem);
em_sem_t* em_sem_new(unsigned startcount, unsigned maxcount);
void em_sem_delete(em_sem_t* sem);
int em_sem_take(em_sem_t* sem, unsigned timeout);
//--------------------------------------------------------------
// These functions can be called both in tasks and in ISR
//--------------------------------------------------------------
static inline int em_sem_try_take(em_sem_t* sem)
{
	return em_sem_take(sem, 0);
}
int em_sem_give(em_sem_t* sem);

#ifdef _EM_TASK_PRIVATE_
em_list_t* rt_em_get_sem_init_list(void);
static inline em_sem_t* rt_em_get_sem_by_init_list(em_list_t* list)
{
	return rt_em_get_struct_by_field(list, em_sem_t, init_list);
}
#endif /* _EM_TASK_PRIVATE_ */

#endif /* _EM_SEM_H_ */
