//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_LIST_H_
#define _EM_LIST_H_

typedef struct _em_list_t
{
	struct _em_list_t *next;
	struct _em_list_t *prev;
} em_list_t;

#define __EM_LIST_INIT(list) em_list_t list = {&list, &list}

#define rt_em_get_struct_by_field(addr_field, type, field)	\
	((type *)((unsigned char *)(addr_field) - (unsigned char *)(&((type *)0)->field)))

void rt_em_list_clear(em_list_t *lst);
int rt_em_is_list_empty(em_list_t *lst);
int rt_em_is_list_whole(em_list_t *lst);
void rt_em_list_add_tail(em_list_t *lst, em_list_t *nlst);
void rt_em_list_add_head(em_list_t *lst, em_list_t *nlst);
void rt_em_list_remove_entry(em_list_t * lst);
void rt_em_list_jump_head(em_list_t *lst);
void rt_em_list_jump_tail(em_list_t *lst);

#endif /* _EM_LIST_H_ */
