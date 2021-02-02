//
//	Author: Shipaev Ivan Vladimirovich
//	iva_n_@mail.ru

#define _EM_TASK_PRIVATE_
#include "task_include.h"

typedef struct _em_memblc_t
{
	struct _em_memblc_t *next;
	struct
	{
		uint32_t size :31;
		uint32_t busy :1;
	};
} em_memblc_t;

#define SIZE_HEAP_BUF		ALLIGN_LEAST(cfgSIZE_HEAP_BUF, cfgMEM_ALLIGN)
#define SIZE_STRUCT_HEAP	ALLIGN_LARGEST(sizeof(em_memblc_t), cfgMEM_ALLIGN)

typedef struct _em_heap_t
{
	union
	{
#if (cfgMEM_ALLIGN == 8)
		uint64_t dummy;
#else
		uint32_t dummy;
#endif
		uint8_t buf[SIZE_HEAP_BUF];
	};
} em_heap_t;

static size_t heap_free_size;
static uint8_t heap_finit;
static em_heap_t heap;

//--------------------------------------------------------------
// Initialization
//--------------------------------------------------------------
void em_mem_init(void)
{
	em_memblc_t *first = (em_memblc_t*) heap.buf;
	first->next = NULL;
	first->size = SIZE_HEAP_BUF;
	first->busy = 0;
	heap_free_size = SIZE_HEAP_BUF;
	heap_finit = 1;
}
//--------------------------------------------------------------
// Malloc
//--------------------------------------------------------------
void* em_malloc(size_t size)
{
	em_memblc_t *cmem, *tmp;

	size = ALLIGN_LARGEST(size + SIZE_STRUCT_HEAP, cfgMEM_ALLIGN);

	em_scheduler_disable();

	if (!heap_finit)
		em_mem_init();

	cmem = (em_memblc_t*) heap.buf;

	while (cmem) {
		if (!cmem->busy && cmem->size >= size) {
			if (cmem->size > size + SIZE_STRUCT_HEAP) {
				// Dividing into small
				tmp = (em_memblc_t*) ((uint8_t*) cmem + size);
				tmp->next = cmem->next;
				tmp->size = cmem->size - size;
				tmp->busy = 0;
				cmem->next = tmp;
				cmem->size = size;
			}
			heap_free_size -= cmem->size;
			cmem->busy = 1;
			break;
		}
		cmem = cmem->next;
	}

#if (sfgSTATISTIC == 1)
	if (cmem == NULL)
		stat_error(STAT_NEW_MEM, size);
#endif

	em_scheduler_enable();

	return cmem ? (uint8_t*) cmem + SIZE_STRUCT_HEAP : NULL;
}
//--------------------------------------------------------------
// Calloc
//--------------------------------------------------------------
void* em_calloc(size_t count, size_t size)
{
	void *p = em_malloc(count * size);
	if (p)
		memset(p, 0, count * size);
	return p;
}
//--------------------------------------------------------------
// Free
//--------------------------------------------------------------
void em_free(void* ptr)
{
	em_memblc_t *cmem, *tmp;

	if (ptr == NULL || !heap_finit)
		return;

	em_scheduler_disable();

	cmem = (em_memblc_t*) heap.buf;
	tmp = NULL;

	while (cmem) {
		if (cmem->busy && ((uint8_t*) cmem + SIZE_STRUCT_HEAP == ptr)) {
			cmem->busy = 0;
			heap_free_size += cmem->size;
			// Combining adjacent
			if (tmp && !tmp->busy) {
				tmp->next = cmem->next;
				tmp->size += cmem->size;
				cmem = tmp;
			}
			tmp = cmem->next;
			if (tmp && !tmp->busy) {
				cmem->next = tmp->next;
				cmem->size += tmp->size;
			}
			break;
		}
		tmp = cmem;
		cmem = cmem->next;
	}

#if (sfgSTATISTIC == 1)
	if (cmem == NULL)
		stat_error(STAT_FREE_MEM, ptr);
#endif

	em_scheduler_enable();
}
//--------------------------------------------------------------
// Get free space
//--------------------------------------------------------------
size_t em_heap_get_free_space(void)
{
	return heap_free_size;
}
