/*
 * Dijkstra-shortest-path.c
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

#include <ctype.h>
#include <limits.h>

#define NUM_VERTEXES (9)

static int s_edges[NUM_VERTEXES][NUM_VERTEXES] = {
//0	0	1	2	3	4	5	6	7	8
	{0, 4, 	0, 	0, 	0, 	0, 	0, 	8, 	0},	// 0
	{4, 0, 	8, 	0, 	0, 	0, 	0, 	11, 0},	// 1
	{0, 8, 	0, 	7, 	0, 	4, 	0, 	0, 	2},	// 2
	{0, 0, 	7, 	0, 	9, 14, 	0, 	0, 	0},	// 3
	{0, 0, 	0, 	9, 	0, 10, 	0, 	0, 	0},	// 4
	{0, 0, 	4, 	0, 	10,	0, 	2, 	0, 	0},	// 5
	{0, 0, 	0, 14, 	0, 	2, 	0, 	1, 	6},	// 6
	{8, 11,	0,	0, 	0, 	0, 	1, 	0, 	7},	// 7
	{0, 0, 	2, 	0, 	0, 	0, 	6, 	7, 	0}	// 8
};

struct vertex_status
{
	int id;
	int min_weight;
	int visited;
	
	int num_candidates;
	int parent_candidates[NUM_VERTEXES];
};

#define WEIGHT_UNSET (INT_MAX)
static struct vertex_status s_vertex_status[NUM_VERTEXES];

static void vertext_status_array_init(struct vertex_status * status_array, size_t length)
{
	memset(status_array, 0, length * sizeof(*status_array));
	for(size_t i = 0; i < length; ++i) {
		status_array[i].id = i;
		status_array[i].min_weight = WEIGHT_UNSET;
		status_array[i].parent_candidates[0] = -1;
	}
}

struct working_queue
{
	int ids[NUM_VERTEXES];	// data
	
	int head;
	int count;
	int cur_pos;
};

int working_queue_init(struct working_queue * queue)
{
	memset(queue, 0, sizeof(*queue));
	for(int i = 0; i < NUM_VERTEXES; ++i) queue->ids[i] = -1;
	
	return 0;
}

int working_queue_push(struct working_queue * queue, int id)
{
	for(int i = 0; i < queue->count; ++i) {
		int pos = queue->head + i;
		pos %= NUM_VERTEXES;
		if(queue->ids[pos] == id) return -1;
	}
	
	queue->ids[queue->cur_pos++] = id;
	queue->cur_pos %= NUM_VERTEXES;
	
	if(queue->count < NUM_VERTEXES) ++queue->count;
	else ++queue->head;
	
	return 0;
}

int working_queue_pop(struct working_queue * queue)
{
	if(queue->count <= 0) return -1;
	
	int id = queue->ids[queue->head++];
	queue->head %= NUM_VERTEXES;
	--queue->count;
	
	return id;
}

int main(int argc, char **argv)
{
	int dst_id = 5;
	int src_id = 0;
	if(argc > 1) dst_id = atoi(argv[1]);
	if(argc > 2) src_id = atoi(argv[2]);
	
	assert(src_id >= 0 && src_id < NUM_VERTEXES);
	assert(dst_id >= 0 && dst_id < NUM_VERTEXES);
	
	struct vertex_status * vstats = s_vertex_status;
	vertext_status_array_init(vstats, NUM_VERTEXES);
	
	vstats[src_id].min_weight = 0;
	
	struct working_queue queue[1];
	working_queue_init(queue);
	working_queue_push(queue, src_id);

	int cur_id = -1;
	
	int found = 0;
	while((cur_id = working_queue_pop(queue)) >= 0) {
		struct vertex_status * current = &vstats[cur_id];
		assert(cur_id == current->id);
		
		if(cur_id == dst_id) {
			found = 1;
			printf("== src skipped: is dst(%d)\n", dst_id);
			continue;
		}
		
		if(found && current->min_weight >= vstats[dst_id].min_weight) {
			printf("== src skipped: [%d], min_weight=%d, greater than dst->min_weight\n", 
				cur_id, current->min_weight);
			continue;
		}
		printf("\e[32m""== pop: id=[%d], min_weight=%d, parent=%d: " "\e[39m" "\n", cur_id, current->min_weight, current->parent_candidates[0]);
		
		int * edges = s_edges[cur_id];
		for(int i = 0; i < NUM_VERTEXES; ++i) {
			if(cur_id == i) continue;
			if(edges[i] <= 0) continue;	// no edges between the twovertexes
			if(vstats[i].visited) continue;
			
			int weight = current->min_weight + edges[i];
			if(weight < vstats[i].min_weight) {
				vstats[i].parent_candidates[0] = cur_id;
				vstats[i].min_weight = weight;
			}
			
			int rc = working_queue_push(queue, i);
			printf("    --> %s: ", (rc==-1)?"\e[33mskip in-queue":"push");
			printf("[%d], edge=%d, weight=%d, min_weigth=%d, parent=%d" "\e[39m\n", i, edges[i], weight, vstats[i].min_weight, vstats[i].parent_candidates[0]);
		}
		current->visited = 1;
		
	}
	
	struct vertex_status *dst = &vstats[dst_id];
	printf("dst_id=%d, min_weight=%d, parent=%d\n", dst->id, dst->min_weight, dst->parent_candidates[0]);
	
	return 0;
}

