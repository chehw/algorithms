/*
 * dijkstra-shortest-path.c
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

#include <search.h>

#include <stdint.h>
#include "algorithms-c-common.h"

#include "dijkstra.h"


/************************************
 * dijkstra_sparse_edge
************************************/

/* use <src_id | dst_id> as key */
static int dijkstra_sparse_edge_compare(const void * _a, const void * _b)
{
	const struct dijkstra_sparse_edge * a = _a;
	const struct dijkstra_sparse_edge * b = _b;

	if(a->src_id > b->src_id) return 1;
	if(a->src_id < b->src_id) return -1;
	if(a->dst_id > b->dst_id) return 1;
	if(a->dst_id < b->dst_id) return -1;
	return 0;
}

/**
 * function sparse_edges_list_add()
 * 
 *   @param row : rows[vertex.id]
 *   @param edge: an edge belongs to the vertex
 * 
 *  @return the pointer of row_edges_array[vertex.id]
**/
static int sparse_edges_compare_weight(const void *_a, const void *_b)
{
	const struct dijkstra_sparse_edge * a = (const struct dijkstra_sparse_edge *)_a;
	const struct dijkstra_sparse_edge * b = (const struct dijkstra_sparse_edge *)_b;
	
	return (a->weight > b->weight)?1:(a->weight < b->weight)?-1:0;
}
static struct clib_sorted_list * sparse_edges_list_add(struct clib_sorted_list * list, const struct dijkstra_sparse_edge * edge)
{
	assert(edge);
	if(NULL == list) {
		// create a new list
		list = clib_sorted_list_init(NULL, sparse_edges_compare_weight, NULL);  
		assert(list);
	}

	int rc = list->add(list, (void *)edge);
	assert(0 == rc);
	return list;
}

/**
 * function sparse_edges_list_remove()
 * 
 *   @param row : list = vertex_edges_array[vertex.id]
 *   @param edge: an edge belongs to the vertex
 * 
 *  @return 0 on success, -1 on failure.
**/
static int sparse_edges_compare_dst_id(const void *_a, const void *_b)
{
	const struct dijkstra_sparse_edge * a = (const struct dijkstra_sparse_edge *)_a;
	const struct dijkstra_sparse_edge * b = (const struct dijkstra_sparse_edge *)_b;
	
	return (a->dst_id > b->dst_id)?1:(a->dst_id < b->dst_id)?-1:0;
}
static int sparse_edges_list_remove(struct clib_sorted_list * list, const struct dijkstra_sparse_edge * edge)
{
	assert(edge);
	if(NULL == list) return -1;
	
	clib_list_iterator_t iter;
	ssize_t count = list->find(list, edge, &iter, sparse_edges_compare_dst_id);
	if(count <= 0) return -1;
	void * data = list->remove(list, &iter);
	assert(data == (void *)edge);
	return 0;
}


/**
 * function dijkstra_edges_get_vertex_sparse_edges()
 *   @param edges:     [IN] a dijkstra_edges object,
 *   @param vertex_id  [IN] vertex.id or vertex_status.index
 *   @param p_edges    [OUT readonly] the pointer of the queue which hold all edges corresponding to the vertex_id. 
 *  @return 
 *     num_edges on success, -1 on failure.
 * 
**/
static ssize_t get_vertex_sparse_edges(struct dijkstra_edges * edges, uint32_t vertex_id, const struct clib_slist ** p_edges)
{
	assert(edges->is_sparse_matrix);
	assert(vertex_id < edges->num_vertices);
	assert(p_edges);
	assert(edges->vertex_edges_array);
	
	*p_edges = edges->vertex_edges_array->data_ptrs[vertex_id];
	if(*p_edges) return (*p_edges)->length;
	return 0;
}

/**
 * function dijkstra_edges_update(): addnew or update an edge
 *   @param edges:  [IN] a dijkstra_edges object,
 *   @param src_id  [IN] the id of the start vertex
 *   @param dst_id  [IN] the id of the end vertex
 *   @param weight  [IN] weigth of this edge
 *  @return 
 *     0 on success, -1 on failure.
 * 
**/
static struct dijkstra_sparse_edge * dijkstra_edges_update(struct dijkstra_edges * edges, uint32_t src_id, uint32_t dst_id, int64_t weight)
{
	if(!edges->is_sparse_matrix) 
	{
		assert(src_id < edges->num_vertices);
		assert(dst_id < edges->num_vertices);
		edges->weights[src_id * edges->num_vertices + dst_id] = weight;
		
		return NULL;
	}
	
	struct dijkstra_sparse_edge * edge = calloc(1, sizeof(*edge));
	assert(edge);
	edge->src_id = src_id;
	edge->dst_id = dst_id;
	
	struct dijkstra_sparse_edge ** p_node = tsearch(edge, &edges->search_root, dijkstra_sparse_edge_compare);
	assert(p_node);
	if(*p_node != edge) { // already exists, ==> update weight only
		free(edge);
		edge = *p_node;
	}
	edge->weight = weight;
	
	// append to row_edges array
	struct clib_pointer_array * vertex_edges_array = edges->vertex_edges_array;
	
	clib_pointer_array_resize(vertex_edges_array, src_id + 1);
	struct clib_sorted_list * list = vertex_edges_array->data_ptrs[src_id];
	vertex_edges_array->data_ptrs[src_id] = sparse_edges_list_add(list, edge);
	if(vertex_edges_array->length <= src_id) vertex_edges_array->length = src_id + 1;
	
	return edge;
}

/**
 * function dijkstra_edges_remove():  remove an edge
 *   @param edges:  [IN] a dijkstra_edges object,
 *   @param src_id  [IN] the id of the start vertex
 *   @param dst_id  [IN] the id of the end vertex
 *   @param weight  [IN] weigth of this edge
 *  @return 
 *     0 on success, -1 on failure.
 * 
**/
static struct dijkstra_sparse_edge * dijkstra_edges_remove(struct dijkstra_edges * edges,  uint32_t src_id, uint32_t dst_id)
{
	if(!edges->is_sparse_matrix) {
		assert(src_id < edges->num_vertices);
		assert(dst_id < edges->num_vertices);
		edges->weights[src_id * edges->num_vertices + dst_id] = -1;
		return NULL;
	}
	struct dijkstra_sparse_edge pattern = {
		.src_id = src_id,
		.dst_id = dst_id
	};
	struct dijkstra_sparse_edge ** p_node = tfind(&pattern, &edges->search_root, dijkstra_sparse_edge_compare);
	if(NULL == p_node) return NULL;
	
	struct dijkstra_sparse_edge * edge = *p_node;
	tdelete(&pattern, &edges->search_root, dijkstra_sparse_edge_compare);
	sparse_edges_list_remove(edges->vertex_edges_array->data_ptrs[src_id], edge);
	return edge;
}

static int64_t dijkstra_edges_get_weight(struct dijkstra_edges * edges,  uint32_t src_id, uint32_t dst_id)
{
	int64_t weight = -1;
	if(!edges->is_sparse_matrix) {
		assert(src_id < edges->num_vertices);
		assert(dst_id < edges->num_vertices);
		weight = edges->weights[src_id * edges->num_vertices + dst_id];
		return weight;
	}
	
	struct dijkstra_sparse_edge pattern = {
		.src_id = src_id,
		.dst_id = dst_id
	};
	struct dijkstra_sparse_edge ** p_node = tfind(&pattern, &edges->search_root, dijkstra_sparse_edge_compare);
	if(NULL == p_node) return -1;
	
	struct dijkstra_sparse_edge * edge = *p_node;
	assert(edge);
	weight = edge->weight;
	
	return weight;
}

struct dijkstra_edges * dijkstra_edges_init(struct dijkstra_edges * edges, int is_sparse_matrix, uint32_t num_vertices)
{
	if(NULL == edges) edges = calloc(1, sizeof(*edges));
	else memset(edges, 0, sizeof(*edges));
	
	edges->is_sparse_matrix = is_sparse_matrix;
	edges->num_vertices = num_vertices;
	
	edges->update = dijkstra_edges_update;
	edges->remove = dijkstra_edges_remove;
	edges->get_weight = dijkstra_edges_get_weight;
	edges->get_vertex_sparse_edges = get_vertex_sparse_edges;
	
	if(is_sparse_matrix) {
		assert(num_vertices > 0);
		clib_pointer_array_init(edges->vertex_edges_array, num_vertices);
		assert(edges->vertex_edges_array->data_ptrs);
	}
	
	return edges;
}


static void free_sorted_list(void * list)
{
	clib_sorted_list_clear(list);
	free(list);
}

void dijkstra_edges_cleanup(struct dijkstra_edges * edges)
{
	if(NULL == edges) return;
	if(!edges->is_sparse_matrix) {
		free(edges->weights);
		edges->weights = NULL;
	}else {
		assert(edges->num_vertices <= edges->vertex_edges_array->max_size);
		
		edges->vertex_edges_array->length = edges->num_vertices;
		clib_pointer_array_cleanup(edges->vertex_edges_array, free_sorted_list);
		
		tdestroy(edges->search_root, free);
		edges->search_root = NULL;
	}
	return;
}

/************************************
 * dijkstra_vertex_status
************************************/
void dijkstra_clear_status_array(struct dijkstra_context * dijkstra)
{
	assert(dijkstra);
	if(dijkstra->status_array) {
		free(dijkstra->status_array);
		dijkstra->status_array = NULL;
	}
}

void dijkstra_vertex_status_dump(const struct dijkstra_vertex_status * status)
{
	assert(status);
	printf("vertex.id=%u, min_weight=%ld, amount=%ld, "
		"visited=%d, is_processing=%d, depth=%d, parent_id=%d\n",
		status->id, (long)status->min_weight, (long)status->amount,
		status->visited, status->is_processing,
		(int)status->depth,
		status->parent?(int)status->parent->id:-1);
}

ssize_t dijkstra_shortest_path(
	struct dijkstra_context * dijkstra, 
	uint32_t src_id, uint32_t dst_id,
	struct clib_pointer_array * candidates)
{
	assert(dijkstra && dijkstra->graph);
	assert(src_id < dijkstra->graph->num_vertices);
	assert(dst_id < dijkstra->graph->num_vertices);
	
	const struct dijkstra_graph * graph = dijkstra->graph;
	struct dijkstra_edges * edges = (struct dijkstra_edges *)graph->edges;

	struct clib_queue queue[1];
	clib_queue_init(queue);
	dijkstra_clear_status_array(dijkstra);

	// step 0. init status_array
	struct dijkstra_vertex_status * status_array = calloc(graph->num_vertices, sizeof(*status_array));
	for(uint32_t i = 0; i < graph->num_vertices; ++i) {
		struct dijkstra_vertex_status *status = &status_array[i];
		status->vertex = &graph->vertices[i];
		status->id = i;
		status->min_weight = DIJKSTRA_WEIGHT_UNSET;
		status->amount = INT64_MAX;
		
		status->visited = 0;
		status->is_processing = 0;
	}
	dijkstra->status_array = status_array;
	
	// step 1. push vertices[src_id] to working queue
	struct dijkstra_vertex_status * vertex = &status_array[src_id];
	vertex->amount = dijkstra->amount;
	
	struct dijkstra_vertex_status * dst_status = &status_array[dst_id];

	vertex->min_weight = 0;
	queue->enter(queue, vertex);
	vertex->is_processing = 1;

	int found = 0;
	struct dijkstra_vertex_status * current = NULL;
	while((current = queue->leave(queue)))
	{
		//~ if(current->id == dst_id) {	// found a path
			//~ found = 1;
			//~ continue;
		//~ }
		debug_printf("====  current: "); dijkstra_vertex_status_dump(current);
		if(found && current->min_weight > dst_status->min_weight) {
			debug_printf("  --> skipped [%u], min_weight=%ld\n", current->id, (long)current->min_weight);
			continue;
		}
		
		// step 2. get all edges belong to the vertex
		struct clib_slist * vertex_edges = NULL;
		ssize_t count = edges->get_vertex_sparse_edges(edges, current->id, (const struct clib_slist **)&vertex_edges);
		if(count <= 0) continue;
		
		debug_printf("  -- edges-count=%d\n", (int)count);
		
		
		// step 3. update min_weight
		clib_list_iterator_t iter;
		memset(&iter, 0, sizeof(iter));
		_Bool ok = clib_slist_iter_begin(vertex_edges, &iter);
		while(ok) {
			struct dijkstra_sparse_edge * edge = clib_list_iterator_get_data(iter);
			assert(edge);
			assert(edge->dst_id < edges->num_vertices);
			vertex = &status_array[edge->dst_id];
			
			if(vertex->id == dst_id) found = 1;
			
			if(vertex->visited) {
				ok = clib_slist_iter_next(vertex_edges, &iter);
				continue;
			}
			
			int64_t weight = INT64_MAX;
			
			
			if(dijkstra->calc_weight) {
				weight = current->min_weight + dijkstra->calc_weight(current->amount, edge->user_data);
			}else {
				weight = current->min_weight + edge->weight;
			}
			if(weight < vertex->min_weight) {
				vertex->min_weight = weight;
				vertex->parent = current;
				vertex->depth = current->depth + 1;
				
				if(dijkstra->calc_amount) vertex->amount = dijkstra->calc_amount(current->amount, edge->user_data);
			}
			debug_printf("    -- next hop: "); dijkstra_vertex_status_dump(vertex);
			
			// step 4. push unprocessed vertex into queue
			if(!vertex->is_processing) {
				vertex->is_processing = 1;
				debug_printf("\e[32m         ==> push [%d]\e[39m\n", (int)vertex->id);
				queue->enter(queue, vertex);
			}
			
			
			ok = clib_slist_iter_next(vertex_edges, &iter);
		}
		current->visited = 1;
	}
	
	clib_queue_clear(queue, NULL);
	
	// get path
	if(found && candidates)
	{
		struct dijkstra_vertex_status * vertex = dst_status;
		assert(dst_status->depth >= 0);
		clib_pointer_array_clear(candidates, NULL);
		clib_pointer_array_set_length(candidates, dst_status->depth + 1);
		
		do {
			debug_printf("[%d] <== ", (int)vertex->id);
			assert(vertex->depth >= 0 && vertex->parent->depth == (vertex->depth - 1));
			
			candidates->data_ptrs[vertex->depth] = vertex;
			vertex = (struct dijkstra_vertex_status *)vertex->parent;
		}while(vertex && vertex->id != src_id);
		
		assert(vertex->id == src_id);
		vertex = &status_array[src_id];
		debug_printf("[%d]\n", (int)vertex->id);
		
		candidates->data_ptrs[0] = vertex;
	}
	return found?dst_status->min_weight:-1;
}


/************************************
 * dijkstra_context
************************************/
struct dijkstra_context * dijkstra_context_init(struct dijkstra_context * dijkstra, 
	const struct dijkstra_graph * graph,
	void * user_data)
{
	assert(graph);
	//~ assert(graph->vertices);
	assert(graph->num_vertices > 0);
	assert(graph->edges);
	
	if(NULL == dijkstra) dijkstra = calloc(1, sizeof(*dijkstra));
	else memset(dijkstra, 0, sizeof(*dijkstra));
	assert(dijkstra);
	
	dijkstra->graph = graph;
	dijkstra->user_data = user_data;
	dijkstra->shortest_path = dijkstra_shortest_path;
	
	
	return dijkstra;
}

void dijkstra_context_cleanup(struct dijkstra_context * dijkstra)
{
	if(NULL == dijkstra) return;
	dijkstra_clear_status_array(dijkstra);
	return;
}


/****************************************************
 * TEST_MODULE::dijkstra-shortest-path
 * build: 
 *   tests/make.sh dijkstra-shortest-path
****************************************************/
#if defined(TEST_DIJKSTRA_SHORTEST_PATH) && defined(ALGORITHMS_C_STAND_ALONE)

// sample-data
#define NUM_VERTEXES (9)
static int64_t s_edges[NUM_VERTEXES][NUM_VERTEXES] = {
//	  0    1    2    3    4    5    6    7    8
	{-1,   4,  -1,  -1,  -1,  -1,  -1,   8,  -1},	// 0
	{ 4,  -1,   8,  -1,  -1,  -1,  -1,  11,  -1},	// 1
	{-1,   8,  -1,  7,   -1,   4,  -1,  -1,   2},	// 2
	{-1,  -1,   7,  -1,   9,  14,  -1,  -1,  -1},	// 3
	{-1,  -1,  -1,   9,  -1,  10,  -1,  -1,  -1},	// 4
	{-1,  -1,   4,  -1,  10,  -1,   2,  -1,  -1},	// 5
	{-1,  -1,  -1,  14,  -1,   2,  -1,   1,   6},	// 6
	{ 8,  11,  -1,  -1,  -1,  -1,   1,  -1,   7},	// 7
	{-1,  -1,   2,  -1,  -1,  -1,   6, 	 7,  -1}	// 8
};

static void print_edge(const void * p_node, VISIT which, int depth)
{
	const struct dijkstra_sparse_edge * edge;
	switch(which) {
	case preorder: case endorder: break;
	case postorder: case leaf:
		edge = *(struct dijkstra_sparse_edge **)p_node;
		printf("edge[%u -> %u]: weight=%ld\n", edge->src_id, edge->dst_id, (long)edge->weight);
		break;
	}
	return;
}

static void sparse_edges_dump(struct dijkstra_edges * edges)
{
	twalk(edges->search_root, print_edge);
}

static void sparse_edges_list_dump(struct dijkstra_edges * edges)
{
	struct clib_pointer_array * edges_array = edges->vertex_edges_array;
	for(size_t i = 0; i < edges_array->length; ++i)
	{
		printf("  [%zu]: ", i);
		struct clib_slist * list = edges_array->data_ptrs[i];
		if(list) {
			printf("(count=%d), ", (int)list->length);
			clib_list_iterator_t iter;
			memset(&iter, 0, sizeof(iter));
			_Bool ok = clib_slist_iter_begin(list, &iter);
			while(ok) {
				struct dijkstra_sparse_edge * edge = clib_list_iterator_get_data(iter);
				assert(edge);
				printf(" [%u]=%ld,", edge->dst_id, (long)edge->weight);
				
				ok = clib_slist_iter_next(list, &iter);
			}
			
			printf("\n");
		}else printf("(empty)\n");
		
	}
	
}

int main(int argc, char **argv)
{
	uint32_t src_id = 0;
	uint32_t dst_id = 5;
	if(argc > 1) src_id = atoi(argv[1]);
	if(argc > 2) dst_id = atoi(argv[2]);
	printf("find path: from [%u] to [%u] ...\n", src_id, dst_id);
	
	// 0. init vertices and edges
	struct dijkstra_vertex vertices[NUM_VERTEXES]; 
	memset(vertices, 0, sizeof(vertices));
	for(size_t i = 0; i < NUM_VERTEXES; ++i) vertices[i].id = i;
	
	struct dijkstra_edges edges[1];
	dijkstra_edges_init(edges, 1, NUM_VERTEXES);
	
	for(uint32_t src_id = 0; src_id < NUM_VERTEXES; ++src_id)
	{
		for(uint32_t dst_id = 0; dst_id < NUM_VERTEXES; ++dst_id) {
			int64_t weight = s_edges[src_id][dst_id];
			if(weight <= 0) continue;
			edges->update(edges, src_id, dst_id, weight);
		}
	}
	assert(edges->vertex_edges_array->length == NUM_VERTEXES);
	sparse_edges_dump(edges);
	sparse_edges_list_dump(edges);
	
	struct dijkstra_graph graph[1];
	memset(graph, 0, sizeof(graph));
	graph->vertices = vertices;
	graph->num_vertices = NUM_VERTEXES;
	graph->edges = edges;
	
	struct dijkstra_context dijkstra[1];
	memset(dijkstra, 0, sizeof(dijkstra));
	dijkstra_context_init(dijkstra, graph, NULL);
	
	struct clib_pointer_array first_candidates[1];
	memset(first_candidates, 0, sizeof(first_candidates));
	
	int64_t min_weight = dijkstra_shortest_path(dijkstra, src_id, dst_id, first_candidates);
	printf("min_weight: %ld\n", (long)min_weight);
	
	// print path
	
	printf("==== path: hops=%d ====\n", (int)(first_candidates->length - 1));
	for(size_t i = 0; i < first_candidates->length; ++i) {
		const struct dijkstra_vertex_status * status = first_candidates->data_ptrs[i];
		assert(status);
		if(i == 0) printf("\e[34mvertices[%d](min_weight=%d)\e[39m", (int)status->id, (int)status->min_weight);
		else printf("vertices[%d](min_weight=%d)", (int)status->id, (int)status->min_weight);
		if(i < (first_candidates->length - 1)) printf(" ==> ");
	}
	printf("\n");
	
	clib_pointer_array_clear(first_candidates, NULL);
	
	/// reuse dijkstra to check memory leak
	min_weight = dijkstra_shortest_path(dijkstra, src_id, dst_id, first_candidates);
	printf("min_weight: %ld\n", (long)min_weight);
	
	// print path
	
	printf("==== path: hops=%d ====\n", (int)(first_candidates->length - 1));
	for(size_t i = 0; i < first_candidates->length; ++i) {
		const struct dijkstra_vertex_status * status = first_candidates->data_ptrs[i];
		assert(status);
		if(i == 0) printf("\e[34mvertices[%d](min_weight=%d)\e[39m", (int)status->id, (int)status->min_weight);
		else printf("vertices[%d](min_weight=%d)", (int)status->id, (int)status->min_weight);
		if(i < (first_candidates->length - 1)) printf(" ==> ");
	}
	printf("\n");
	
	clib_pointer_array_cleanup(first_candidates, NULL);	
	dijkstra_edges_cleanup(edges);
	dijkstra_context_cleanup(dijkstra);
	return 0;
}
#endif

