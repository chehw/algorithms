/*
 * clib-array.c
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
#define CLIB_POINTER_ARRAY_ALLOC_SIZE (4096)

int clib_pointer_array_resize(struct clib_pointer_array * array, size_t new_size)
{
	if(0 == new_size) new_size = CLIB_POINTER_ARRAY_ALLOC_SIZE;
	else new_size = (new_size + CLIB_POINTER_ARRAY_ALLOC_SIZE - 1) / CLIB_POINTER_ARRAY_ALLOC_SIZE * CLIB_POINTER_ARRAY_ALLOC_SIZE;
	if(new_size <= array->max_size) return 0;
	
	void ** data_ptrs = realloc(array->data_ptrs, sizeof(void *) * new_size);
	assert(data_ptrs);
	memset(data_ptrs + array->max_size, 0, (new_size - array->max_size) * sizeof(void *));
	
	array->data_ptrs = data_ptrs;
	array->max_size = new_size;
	return 0;
}

int clib_pointer_array_set_length(struct clib_pointer_array * array, size_t new_length)
{
	if(clib_pointer_array_resize(array, new_length)) return -1;
	array->length = new_length;
	return 0;
}

struct clib_pointer_array * clib_pointer_array_init(struct clib_pointer_array * array, size_t size)
{
	if(NULL == array) array = calloc(1, sizeof(*array));
	else memset(array, 0, sizeof(*array));
	assert(array);
	clib_pointer_array_resize(array, size);
	return array;
}

void clib_pointer_array_clear(struct clib_pointer_array * array, void (*free_data)(void *))
{
	if(NULL == array || NULL == array->data_ptrs) return;
	if(free_data) {
		for(size_t i = 0; i < array->length; ++i) {
			void * data = array->data_ptrs[i];
			free_data(data);
			array->data_ptrs[i] = NULL;
		}
	}
	return;
}

void clib_pointer_array_cleanup(struct clib_pointer_array * array, void (*free_data)(void *))
{
	if(NULL == array) return;
	clib_pointer_array_clear(array, free_data);
	
	if(array->data_ptrs) {
		free(array->data_ptrs);
		array->data_ptrs = NULL;
	}
	
	array->max_size = 0;
	array->length = 0;
	return;
}

static int circular_array_pop(struct clib_circular_array * array)
{
	assert(array && array->size > 0 && array->start_pos < array->size);

	if(array->length <= 0) return -1;	
	void ** data_ptrs = array->base->data_ptrs;
	assert(data_ptrs);
	
	void * data = data_ptrs[array->start_pos];
	if(array->free_data) array->free_data(data);
	
	data_ptrs[array->start_pos] = NULL;
	
	++array->start_pos;
	--array->length;
	array->start_pos %= array->size;
	
	return 0;
}


static int circular_array_resize(struct clib_circular_array * array, size_t new_size)
{
	if(new_size == 0) new_size = CLIB_POINTER_ARRAY_ALLOC_SIZE;
	if(new_size == array->size) return 0;
	
	int rc = clib_pointer_array_resize(array->base, new_size);
	assert(0 == rc);
	
	void ** data_ptrs = array->base->data_ptrs;
	if(new_size < array->size) {
		if(new_size <= array->length) {  // shrink array and free data
			size_t new_length = new_size;
			while(array->length > new_length) {
				circular_array_pop(array);
			}
			array->length = new_length;
		}
	}
	
	// move data when needed
	size_t new_start_pos = array->start_pos;
	if(new_start_pos >= new_size) new_start_pos = 0;
	
	for(size_t i = 0; i < array->length; ++i) {
		size_t old_pos = (array->length - i - 1 + array->start_pos) % array->size;
		size_t new_pos = (array->length - i - 1 + new_start_pos) % new_size;
		if(old_pos == new_pos) break;
		
		void * data = data_ptrs[old_pos];
		data_ptrs[old_pos] = NULL;
		data_ptrs[new_pos] = data;
	}
	
	array->size = new_size;
	array->start_pos = new_start_pos;
	return 0;
}

static int circular_array_append(struct clib_circular_array * array, void * data)
{
	assert(array && array->size > 0);
	size_t pos = (array->start_pos + array->length) % array->size;
	
	array->base->data_ptrs[pos] = data;
	if(array->length < array->size) ++array->length;
	else {
		++array->start_pos;
		array->start_pos %= array->size;
	}
	
	return 0;
}

static void * circular_array_get(struct clib_circular_array * array, size_t index)
{
	assert(array && array->size > 0);
	if(index > array->length) return NULL;
	
	size_t pos = (index + array->start_pos) % array->size;
	void * data = array->base->data_ptrs[pos];
	return data;
}
static int circular_array_set(struct clib_circular_array * array, size_t index, void * data)
{
	assert(array && array->size > 0);
	if(index > array->length) return -1;
	
	size_t pos = (index + array->start_pos) % array->size;
	array->base->data_ptrs[pos] = data;
	return 0;
}

struct clib_circular_array * clib_circular_array_init(struct clib_circular_array * array, size_t size, void (*free_data)(void *))
{
	if(NULL == array) array = calloc(1, sizeof(*array));
	else memset(array, 0, sizeof(*array));
	assert(array);
	
	clib_pointer_array_init(array->base, size);
	array->size = size;
	
	array->free_data = free_data;
	array->resize = circular_array_resize;
	
	array->append = circular_array_append;
	array->get = circular_array_get;
	array->set = circular_array_set;
	
	return array;
}
void clib_circular_array_cleanup(struct clib_circular_array * array, void (*free_data)(void *))
{
	if(NULL == free_data) free_data = array->free_data;
	clib_pointer_array_cleanup(array->base, free_data);
}
