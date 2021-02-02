//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#include <stddef.h>
#include "em_list.h"
//--------------------------------------------------------------
//  Clear
//--------------------------------------------------------------
void rt_em_list_clear(em_list_t *lst)
{
	lst->prev = lst->next = lst;
}
//--------------------------------------------------------------
// Is empty?
//--------------------------------------------------------------
int rt_em_is_list_empty(em_list_t* lst)
{
	return (lst->next == lst);
}
//--------------------------------------------------------------
// Is whole?
//--------------------------------------------------------------
int rt_em_is_list_whole(em_list_t *lst)
{
	if ((lst->next == NULL) || (lst->prev == NULL))
		return 0;
	return 1;
}
//--------------------------------------------------------------
// Add to tail
//--------------------------------------------------------------
void rt_em_list_add_tail(em_list_t *lst, em_list_t *nlst)
{
	nlst->prev = lst->prev;
	nlst->next = lst;
	lst->prev->next = nlst;
	lst->prev = nlst;
}
//--------------------------------------------------------------
// Add to head
//--------------------------------------------------------------
void rt_em_list_add_head(em_list_t *lst, em_list_t *nlst)
{
	nlst->next = lst->next;
	nlst->prev = lst;
	lst->next->prev = nlst;
	lst->next = nlst;
}
//--------------------------------------------------------------
// Remove entry
//--------------------------------------------------------------
void rt_em_list_remove_entry(em_list_t * lst)
{
	lst->prev->next = lst->next;
	lst->next->prev = lst->prev;
	lst->prev = lst->next = lst;
}
//--------------------------------------------------------------
// Jump to head
//--------------------------------------------------------------
void rt_em_list_jump_head(em_list_t *lst)
{
	lst->prev->next = lst->next;
	lst->next->prev = lst->prev;
	rt_em_list_add_head(lst->next, lst);
}
//--------------------------------------------------------------
// Jump to tail
//--------------------------------------------------------------
void rt_em_list_jump_tail(em_list_t *lst)
{
	lst->prev->next = lst->next;
	lst->next->prev = lst->prev;
	rt_em_list_add_tail(lst->prev, lst);
}
