/*
 * clib-stack.c
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

static int clib_stack_push(struct clib_stack * stack, void * data)
{
	assert(stack);
	struct clib_pointer_array * array = (struct clib_pointer_array *)stack;
	
	int rc = clib_pointer_array_resize(array, array->length+1);
	if(rc) return -1;
	array->data_ptrs[array->length++]=data;
	return 0;
}

static int clib_stack_pop(struct clib_stack *stack, void **p_data)
{
	assert(stack);
	struct clib_pointer_array * array = (struct clib_pointer_array *)stack;
	if(array->length == 0) return -1;
	
	void * data = array->data_ptrs[--array->length];
	if(p_data) *p_data = data;
	return 0;
}

struct clib_stack * clib_stack_init(struct clib_stack * stack, size_t size)
{
	if(NULL == stack) stack = calloc(1, sizeof(*stack));
	assert(stack);
	clib_pointer_array_init((struct clib_pointer_array *)stack, size);

	stack->push = clib_stack_push;
	stack->pop = clib_stack_pop;
	return stack;
}

void clib_stack_clear(struct clib_stack * stack, void (*free_data)(void *))
{
	clib_pointer_array_cleanup((struct clib_pointer_array *)stack, free_data);
	return;
}
