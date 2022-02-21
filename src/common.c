/*
 * common.c
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

#include <stdint.h>
#include "algorithms-c-common.h"

/***************************************
 * TEST MODULE::algorithms-c-common
 * build: 
     $ tests/make.sh common
 *
***************************************/
#if defined(TEST_ALGORITHMS_C_COMMON) && defined(ALGORITHMS_C_STAND_ALONE)
static void clib_slist_dump(struct clib_slist *list, const char * title)
{
	assert(list);
	clib_list_iterator_t iter;
	printf("  %s: ", title);
	_Bool ok = clib_slist_iter_begin(list, &iter);
	if(!ok) return;
	
	do{
		struct clib_slist_node * node = iter.current;
		printf("--> (%p)%ld ", node, (long)(intptr_t)node->data);
	}while(clib_slist_iter_next(list, &iter));
	printf("\n");
	
}

static void fill_sample_data(struct clib_slist * list)
{
	struct clib_slist_node * node = calloc(1, sizeof(*node));
	node->data = (intptr_t)0;
	list->head = node;
	int count;
	
	for(count = 1; count < 4; ++count) {
		node->next = calloc(1, sizeof(*node));
		node = node->next;
		node->data = (void *)(intptr_t)count;
	}
	list->tail = node;
	list->length = count;
	return;
}

static void test_slist_reverse(void)
{
	struct clib_slist list[1];
	memset(list, 0, sizeof(list));
	
	printf("\e[33m===== %s =====\e[39m\n", __FUNCTION__);
	// init list
	fill_sample_data(list);
	
	clib_slist_dump(list, "origin");
	
	// reverse list
	clib_slist_reverse(list);
	clib_slist_dump(list, "reversed");
	
	clib_slist_clear(list, NULL);
}

static void test_pointer_array(void)
{
	printf("===== %s =====\n", __FUNCTION__);
	
	struct clib_pointer_array array[1];
	clib_pointer_array_init(array, 0);
	
	const int length = 100;
	// fill_sample_data
	for(int i = 0; i < length; ++i) {
		if(array->length >= array->max_size) clib_pointer_array_resize(array, array->length + 1);
		void ** p_data = &array->data_ptrs[i];
		*p_data = (void *)(intptr_t)i;
		++array->length;
	}
	
	// dump data
	printf("  array: length=%Zu, max_size=%Zu\n", array->length, array->max_size);
	for(int i = 0; i < array->length; ++i) {
		void *data = array->data_ptrs[i];
		printf("    [%.3d]: %d\n", i, (int)(intptr_t)data);
	}
	
	clib_pointer_array_cleanup(array, NULL);
}

static void test_stack(void)
{
	printf("\e[33m===== %s =====\e[39m\n", __FUNCTION__);
	struct clib_stack stack[1];
	
	const int size = 100;
	clib_stack_init(stack, size);
	
	printf("  initial size: %d\n", size);
	for(int i = 0; i < size * 2; ++i) {
		stack->push(stack, (void *)(intptr_t)i);
	}
	printf("  final size: %d\n", (int)stack->base->length);
	
	printf("pop data: ");
	void * data = NULL;
	
	while((0 == stack->pop(stack, &data)))
	{
		printf(" (%.3d) ", (int)(intptr_t)data);
	}
	printf("\n");
	clib_stack_clear(stack, NULL);
}

static void test_circular_array(void)
{
	printf("\e[33m===== %s =====\e[39m\n", __FUNCTION__);
	struct clib_circular_array array[1];
	clib_circular_array_init(array, 8, NULL);
	
	for(int i = 0; i < 10; ++i) {
		array->append(array, (void *)(intptr_t)i);
	}
	
	printf("  origin: size=%zu, start_pos=%d, length=%d\n", array->size, (int)array->start_pos, (int)array->length);
	for(int i = 0; i < array->length; ++i) {
		printf("  data[%d] = %d\n", i, (int)(intptr_t)array->get(array, i));
	} 
	
	array->resize(array, 100);
	printf("  resized: size=%zu, start_pos=%d, length=%d\n", array->size, (int)array->start_pos, (int)array->length);	
	for(int i = 0; i < array->length; ++i) {
		printf("  data[%d] = %d\n", i, (int)(intptr_t)array->get(array, i));
	} 
	
	array->resize(array, 4);
	printf("  resized: size=%zu, start_pos=%d, length=%d\n", array->size, (int)array->start_pos, (int)array->length);
	
	for(int i = 0; i < array->length; ++i) {
		printf("  data[%d] = %d\n", i, (int)(intptr_t)array->get(array, i));
	} 
	
	
	clib_circular_array_cleanup(array, NULL);
}


static int compare_data(const void * _a, const void * _b)
{
	int a = (int)(intptr_t)_a;
	int b = (int)(intptr_t)_b;
	return a - b;
}
static void test_sorted_list(void)
{
	printf("\e[33m===== %s =====\e[39m\n", __FUNCTION__);
	struct clib_sorted_list list[1];
	clib_sorted_list_init(list, compare_data, NULL);

	list->add(list, (void *)(intptr_t)9);
	list->add(list, (void *)(intptr_t)7);
	list->add(list, (void *)(intptr_t)1);
	list->add(list, (void *)(intptr_t)5);
	list->add(list, (void *)(intptr_t)3);
	
	list->add(list, (void *)(intptr_t)2);
	list->add(list, (void *)(intptr_t)6);
	list->add(list, (void *)(intptr_t)4);
	list->add(list, (void *)(intptr_t)8);
	list->add(list, (void *)(intptr_t)10);
	
	clib_slist_dump(list->base, "sorted_list");
	
	
	clib_list_iterator_t iter;
	memset(&iter, 0, sizeof(iter));
	ssize_t count = list->find(list, (void *)(intptr_t)5, &iter, compare_data);
	if(count) {
		list->remove(list, &iter);
	}
	
	count = list->find(list, (void *)(intptr_t)3, &iter, compare_data);
	if(count) {
		list->remove(list, &iter);
	}
	
	clib_slist_dump(list->base, "[remove 5 and 3]");
	clib_sorted_list_clear(list);
}

int main(int argc, char ** argv)
{
	if(0) test_slist_reverse();
	if(0) test_pointer_array();
	if(0) test_stack();
	if(0) test_circular_array();
	if(1) test_sorted_list();
	return 0;
}
#endif
