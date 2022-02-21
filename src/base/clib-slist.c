/*
 * clib-slist.c
 * 
 * Copyright 2022 chehw <hongwei.che@gmail.com>
 * 
 * The MIT License (MIT)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "algorithms-c-common.h"
#include <stdint.h>
#include <stdbool.h>

/***************************************
 * clib_slist
***************************************/
void clib_slist_clear(struct clib_slist * list, void (*free_data)(void *))
{
	if(NULL == list) return;
	
	if(list->is_xor_list) { // convert to normal list
		struct clib_slist_node * current = list->tail;
		struct clib_slist_node * prev = NULL;
		while(current) {
			current->next = (struct clib_slist_node *)((uintptr_t)prev ^ (uintptr_t)current->next);
			prev = current;
			current = current->next;
		}
		
		// swap head and tail
		prev = list->head;
		list->head = list->tail;
		list->tail = prev;
		
		list->is_xor_list = 0;
	}
	
	struct clib_slist_node * node = list->head;
	while(node)
	{
		struct clib_slist_node * next = node->next;
		if(free_data && node->data) {
			free_data(node->data);
			node->data = NULL;
		}
		
		free(node);
		node = next;
	}
	
	list->length = 0;
	list->head = NULL;
	list->tail = NULL;
	return;
}

int clib_slist_push(struct clib_slist *list, void * data)
{
	struct clib_slist_node * node = calloc(1, sizeof(*node));
	assert(node);
	node->data = data;
	node->next = NULL;
	
	++list->length;
	if(NULL == list->head) {
		list->head = list->tail = node;
		return 0;
	}
	
	list->tail->next = node;
	list->tail = node;
	return 0;
}



int clib_slist_convert_to_xor_list(struct clib_slist * list)
{
	if(NULL == list || NULL == list->head) return -1;
	struct clib_slist_node * current = list->head;
	struct clib_slist_node * prev = NULL;
	struct clib_slist_node * next = NULL;
	
	// convert to XOR list
	while(current) {
		next = current->next;
		current->next = (struct clib_slist_node *)((uintptr_t)prev ^ (uintptr_t)current->next);
		prev = current;
		current = next;
	}
	
	list->is_xor_list = 1;
	return 0;
}

int clib_slist_reverse(struct clib_slist * list)
{
	if(NULL == list || NULL == list->head) return -1;
	
	clib_slist_convert_to_xor_list(list);
	
	// reverse
	struct clib_slist_node * current = list->tail;
	struct clib_slist_node * prev = NULL;
	while(current) {
		current->next = (struct clib_slist_node *)((uintptr_t)prev ^ (uintptr_t)current->next);
		prev = current;
		current = current->next;
	}
	
	// swap head and tail
	prev = list->head;
	list->head = list->tail;
	list->tail = prev;
	
	list->is_xor_list = 0;
	return 0;
}

_Bool clib_slist_iter_next(struct clib_slist * list, clib_list_iterator_t * p_iter)
{
	assert(p_iter);
	
	clib_list_iterator_t iter = *p_iter;
	if(NULL == iter.current) return clib_slist_iter_begin(list, p_iter);
	
	if(list->is_xor_list) {
		// prev --> current --> next 
		// current --> next --> next-next
		
		struct clib_slist_node * next = NULL;
		if(iter.next) next = (struct clib_slist_node *)((uintptr_t)iter.next->next ^ (uintptr_t)iter.current);
		iter.prev = iter.current;
		iter.current = iter.next;
		iter.next = next;
	}else
	{
		iter.prev = iter.current;
		iter.current = iter.current?iter.current->next:NULL;
		iter.next = iter.current?iter.current->next:NULL;
	}
	
	*p_iter = iter;
	list->iter = iter;
	return (NULL != list->iter.current);
}

_Bool clib_slist_iter_begin(struct clib_slist * list, clib_list_iterator_t * p_iter)
{
	if(NULL == list->head) return false;

	list->iter.current = list->head;
	list->iter.prev = NULL;
	list->iter.next = list->head->next;	
	*p_iter = list->iter;
	return (list->iter.current != NULL);
}

/***************************************
 * clib_sorted_list
***************************************/

#define xor_list_node_get_next(prev, current) \
	(struct clib_slist_node *)((uintptr_t)(prev) ^ (uintptr_t)((current)?(current)->next:NULL))


static int sorted_list_add(struct clib_sorted_list * list, void * data)
{
	assert(list && list->base->is_xor_list);
	
	struct clib_slist_node * new_node = calloc(1, sizeof(*new_node));
	assert(new_node);
	new_node->data = data;
	
	clib_list_iterator_t iter;
	memset(&iter, 0, sizeof(iter));
	
	_Bool ok = clib_slist_iter_begin(list->base, &iter);
	if(!ok) { // empty list
		list->base->head = list->base->tail = new_node;
		++list->base->length;
		return 0;
	}
	
	do {
		if(list->compare(data, clib_list_iterator_get_data(iter)) < 0) {
			/* prev -> (new_node) --> current --> next */
			
			//~ debug_printf("  --> add(data=%d): %p(%d) --> \e[33m%p(%d)\e[39m --> %p(%d) --> %p \n", 
				//~ (int)(intptr_t)data,
				//~ iter.prev, (int)(iter.prev?(intptr_t)iter.prev->data:-1),
				//~ new_node, (int)(intptr_t)new_node->data,
				//~ iter.current, (int)(iter.current?(intptr_t)iter.current->data:-1),
				//~ iter.next);
			if(iter.prev) {
				iter.prev->next = (struct clib_slist_node *)((uintptr_t)iter.prev->next ^ (uintptr_t)iter.current ^ (uintptr_t)new_node);
			}else {
				list->base->head = new_node;
				new_node->next = list->base->head;
			}
			new_node->next = (struct clib_slist_node *)((uintptr_t)iter.prev ^ (uintptr_t)iter.current);
			iter.current->next = (struct clib_slist_node *)((uintptr_t)new_node ^ (uintptr_t)iter.next);
			break;
		}
	}while(clib_slist_iter_next(list->base, &iter));
	
	
	if(iter.current == NULL) {
		/* prev -> tail --> (new_node) --> null */
		struct clib_slist_node * tail = list->base->tail;
		assert(tail);
		struct clib_slist_node * prev = tail->next; // (tail->next ^ NULL)
		tail->next = (struct clib_slist_node *)((uintptr_t)prev ^ (uintptr_t)new_node);
		new_node->next = tail;  // (tail->next ^ NULL)
		list->base->tail = new_node;
	}
	++list->base->length;
	return 0;
	
}

static void * sorted_list_remove(struct clib_sorted_list * list, clib_list_iterator_t *p_iter)
{
	struct clib_slist * base = list->base;
	assert(base->is_xor_list);
	
	clib_list_iterator_t iter = *p_iter;
	struct clib_slist_node * current = iter.current;
	if(NULL == current) return NULL;
	void * data = current->data;
	
	--base->length;
	if(NULL == iter.prev) { // remove head
		struct clib_slist_node * next = xor_list_node_get_next(iter.current, iter.next);
		base->head = iter.next;
		if(base->head) {
			base->head->next = next; // (NULL ^ next)
			if(NULL == next) base->tail = base->head;
		}else {
			base->tail = NULL;
		}
		free(current);
		return data;
	}
	if(NULL == iter.next) { // remove tail
		struct clib_slist_node * prev_prev = xor_list_node_get_next(iter.current, iter.prev);
		assert(prev_prev);
		iter.prev->next = prev_prev; // (prev ^ NULL)
		base->tail = iter.prev;
		
		free(current);
		return data;
	}
	
	/* --> prev -=> (current) --> next --> */
	/* (prev_prev) --> prev --> next --> (next_next)*/
	struct clib_slist_node * prev_prev = xor_list_node_get_next(iter.current, iter.prev);
	struct clib_slist_node * next_next = xor_list_node_get_next(iter.current, iter.next);

	iter.prev->next = (struct clib_slist_node *)((uintptr_t)prev_prev ^ (uintptr_t)iter.next);
	iter.next->next = (struct clib_slist_node *)((uintptr_t)iter.prev ^ (uintptr_t)next_next);
	
	free(current);
	return data;
}

static ssize_t sorted_list_find(struct clib_sorted_list * list, const void * data, clib_list_iterator_t *p_iter, int (*compare)(const void *, const void *))
{
	if(NULL == compare) compare = list->compare;
	memset(p_iter, 0, sizeof(*p_iter));
	
	clib_list_iterator_t iter;
	memset(&iter, 0, sizeof(iter));
	if(!clib_slist_iter_begin(list->base, &iter)) return -1;
	
	do {
		if(0 == compare(data, clib_list_iterator_get_data(iter))) {
			if(p_iter) *p_iter = iter;
			return 1;
		}
	}while(clib_slist_iter_next(list->base, &iter));
	return 0;
}

static int sorted_list_compare_default(const void * a, const void * b)
{
	return (a>b)?1:(a<b)?-1:0;
}
struct clib_sorted_list * clib_sorted_list_init(struct clib_sorted_list * list, int (*compare_fn)(const void*, const void *), void (*free_data)(void *))
{
	if(NULL == list) list = calloc(1, sizeof(*list));
	else memset(list, 0, sizeof(*list));
	assert(list);
	if(NULL == compare_fn) compare_fn = sorted_list_compare_default;
	
	list->compare = compare_fn;
	list->free_data = free_data;
	
	list->add = sorted_list_add;
	list->remove = sorted_list_remove;
	list->find = sorted_list_find;
	
	list->base->is_xor_list = 1;
	return list;
}

void clib_sorted_list_clear(struct clib_sorted_list * list)
{
	clib_slist_clear(list->base, list->free_data);
	return;
}
