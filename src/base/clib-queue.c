/*
 * clib-queue.c
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

static int clib_queue_enter(struct clib_queue * queue, void * data)
{
	assert(queue);
	struct clib_slist * list = (struct clib_slist *)queue;
	struct clib_slist_node * node = calloc(1, sizeof(*node));
	assert(node);
	node->data = data;
	node->next = NULL;
	
	if(NULL == list->head) list->head = node;
	else list->tail->next = node;
	list->tail = node;
	++list->length;
	return 0;
}

static void * clib_queue_leave(struct clib_queue * queue)
{
	assert(queue);
	struct clib_slist * list = (struct clib_slist *)queue;
	if(NULL == list->head) return NULL;
	
	struct clib_slist_node * node = list->head;
	void * data = node->data;
	
	list->head = node->next;
	node->next = NULL;
	--list->length;
	
	if(NULL == list->head) {
		assert(list->length == 0);
		list->tail = NULL;
	}
	free(node);
	return data;
}

struct clib_queue * clib_queue_init(struct clib_queue * queue)
{
	if(NULL == queue) queue = calloc(1, sizeof(*queue));
	else memset(queue, 0, sizeof(*queue));
	assert(queue);
	
	queue->enter = clib_queue_enter;
	queue->leave = clib_queue_leave;
	return queue;
}

void clib_queue_clear(struct clib_queue * queue, void (*free_data)(void *))
{
	if(NULL == queue) return;
	struct clib_slist * list = (struct clib_slist *)queue;
	clib_slist_clear(list, free_data);
	return;
}
