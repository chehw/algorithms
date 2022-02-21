#ifndef ALGORITHMS_C_COMMON_H_
#define ALGORITHMS_C_COMMON_H_

#include <stdbool.h>

#ifndef debug_printf
#ifdef _DEBUG
#include <stdarg.h>
#define debug_printf(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); }while(0) 
#else
#define debug_printf(fmt, ...) do {  }while(0) 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct clib_pointer_array
{
	size_t max_size;
	size_t length;
	void ** data_ptrs;
};
int clib_pointer_array_resize(struct clib_pointer_array * array, size_t new_size);
struct clib_pointer_array * clib_pointer_array_init(struct clib_pointer_array * array, size_t size);
void clib_pointer_array_cleanup(struct clib_pointer_array * array, void (*free_data)(void *));


struct clib_circular_array
{
	struct clib_pointer_array base[1];
	size_t size;
	size_t length;
	size_t start_pos;
	void (* free_data)(void *data);
	int (* resize)(struct clib_circular_array * array, size_t new_size);
	
	int (*append)(struct clib_circular_array * array, void * data);
	void * (*get)(struct clib_circular_array * array, size_t index);
	int (*set)(struct clib_circular_array * array, size_t index, void * data);
	
};
struct clib_circular_array * clib_circular_array_init(struct clib_circular_array * array, size_t size, void (*free_data)(void *));
void clib_circular_array_cleanup(struct clib_circular_array * array, void (*free_data)(void *));

struct clib_slist_node
{
	void * data;
	struct clib_slist_node * next;
};

struct clib_list_iterator
{
	struct clib_slist_node * current;
	struct clib_slist_node * prev;
	struct clib_slist_node * next;
};

typedef struct clib_list_iterator clib_list_iterator_t;
#define clib_list_iterator_get_data(iter) (iter).current->data

struct clib_slist
{
	struct clib_slist_node * head;
	struct clib_slist_node * tail;
	size_t length;
	clib_list_iterator_t iter;
	int is_xor_list;
};

int clib_slist_convert_to_xor_list(struct clib_slist * list);
int clib_slist_reverse(struct clib_slist * list);
void clib_slist_clear(struct clib_slist * list, void (*free_data)(void *));
_Bool clib_slist_iter_next(struct clib_slist * list, clib_list_iterator_t * p_iter);
_Bool clib_slist_iter_begin(struct clib_slist * list, clib_list_iterator_t * p_iter);
int clib_slist_push(struct clib_slist *list, void * data);

struct clib_queue
{
	struct clib_slist base[1];
	int (* enter)(struct clib_queue * queue, void * data);
	void * (*leave)(struct clib_queue * queue);
};
struct clib_queue * clib_queue_init(struct clib_queue * queue);
void clib_queue_clear(struct clib_queue * queue, void (*free_data)(void *));


struct clib_stack
{
	struct clib_pointer_array base[1];
	int (*push)(struct clib_stack *stack, void *data);
	int (*pop)(struct clib_stack *stack, void **p_data);
};
struct clib_stack * clib_stack_init(struct clib_stack * stack, size_t size);
void clib_stack_clear(struct clib_stack * stack, void (*free_data)(void *));


struct clib_sorted_list
{
	struct clib_slist base[1];
	clib_list_iterator_t *iter;
	
	// callbacks
	void (*free_data)(void *);
	int (*compare)(const void *, const void *);
	
	// public methods
	int (*add)(struct clib_sorted_list * list, void * data);
	void * (*remove)(struct clib_sorted_list * list, clib_list_iterator_t *iter);
	ssize_t (*find)(struct clib_sorted_list * list, const void * data, clib_list_iterator_t *p_iter, int (*compare)(const void *, const void *));
};
struct clib_sorted_list * clib_sorted_list_init(struct clib_sorted_list * list, int (*compare_fn)(const void*, const void *), void (*free_data)(void *));
void clib_sorted_list_clear(struct clib_sorted_list * list);

#ifdef __cplusplus
}
#endif
#endif
