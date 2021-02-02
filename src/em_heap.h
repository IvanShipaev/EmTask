//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#ifndef _EM_MEM_HEAP_H_
#define _EM_MEM_HEAP_H_

void em_mem_init(void);
void* em_malloc(size_t size);
void* em_calloc(size_t count, size_t size);
void em_free(void*);
size_t em_heap_get_free_space(void);

#endif /* _EM_MEM_HEAP_H_ */
