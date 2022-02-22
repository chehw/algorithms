/*
 * samples-dijkstra.c
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

#include "dijkstra.h"

struct ln_routing_fee
{
	int enabled;
	int64_t ppm;
	int64_t base;
};

#define MILLION	(1000 * 1000)
#define NUM_VERTEXES (9)

struct ln_routing_fee s_routing_fees[NUM_VERTEXES][NUM_VERTEXES] = {
	// 
	[0] = { // node 0 to node n
		[0] = {},
		[1] = {.enabled = 1, .ppm=200, .base=1000},
		[2] = {.enabled = 0},
		[3] = {.enabled = 1, .ppm=100},
		[4] = {.enabled = 0},
		[5] = {.enabled = 1, .ppm=500},
		[6] = {.enabled = 0},
		[7] = {.enabled = 0},
		[8] = {.enabled = 0},
	},
	
	[1] = { // node 1 to node n
		[0] = {.enabled = 1, .ppm=200, .base=0, },
		[1] = {},
		[2] = {.enabled = 1, .ppm=500, .base=100},
		[3] = {.enabled = 1, .ppm=10, .base=1000},
		[4] = {.enabled = 0},
		[5] = {.enabled = 0},
		[6] = {.enabled = 1, .ppm=0, .base=5000},
		[7] = {.enabled = 0},
		[8] = {.enabled = 0},
	},
	
	[2] = { // node 2 to node n
		[0] = {.enabled = 1, .ppm=100, .base=0, },
		[1] = {.enabled = 1, .ppm=200, .base=1000},
		[2] = {},
		[3] = {.enabled = 1, .ppm=1000, .base=0},
		[4] = {.enabled = 1, .ppm=50, .base=10},
		[5] = {.enabled = 0},
		[6] = {.enabled = 0},
		[7] = {.enabled = 0},
		[8] = {.enabled = 1, .ppm=5000, .base=0},
	},
	
	[3] = { // node 3 to node n
		[0] = {.enabled = 1, .ppm=100, .base=0, },
		[1] = {.enabled = 0},
		[2] = {.enabled = 1, .ppm=3000,},
		[3] = {},
		[4] = {.enabled = 0},
		[5] = {.enabled = 0},
		[6] = {.enabled = 0},
		[7] = {.enabled = 1, .ppm=100, .base=3000},
		[8] = {.enabled = 0},
	},
	
	[4] = {
		[0] = {.enabled = 1, .ppm=100, .base=0, },
		[1] = {.enabled = 1, .ppm=200, .base=1000},
		[2] = {.enabled = 1, .ppm=0, .base=5000},
		[3] = {.enabled = 0},
		[4] = {},
		[5] = {.enabled = 0},
		[6] = {.enabled = 0},
		[7] = {.enabled = 0},
		[8] = {.enabled = 0},
	},
	
	[5] = {
		[0] = {.enabled = 0,},
		[1] = {.enabled = 1, .ppm=300, .base=1},
		[2] = {.enabled = 0},
		[3] = {.enabled = 0},
		[4] = {.enabled = 0},
		[5] = {},
		[6] = {.enabled = 0},
		[7] = {.enabled = 1, .ppm=100, .base=1000},
		[8] = {.enabled = 1, .ppm=500, .base=10},
	},
	
	[6] = {
		[0] = {.enabled = 0,},
		[1] = {.enabled = 1, .ppm=300, .base=1},
		[2] = {.enabled = 1, .ppm=10000, .base=0},
		[3] = {.enabled = 0},
		[4] = {.enabled = 0},
		[5] = {.enabled = 0},
		[6] = {},
		[7] = {.enabled = 1, .ppm=100, .base=1000},
		[8] = {.enabled = 1, .ppm=500, .base=10},
	},
	
	[7] = {
		[0] = {.enabled = 0,},
		[1] = {.enabled = 1, .ppm=300, .base=1},
		[2] = {.enabled = 1, .ppm=10000, .base=0},
		[3] = {.enabled = 0},
		[4] = {.enabled = 0},
		[5] = {.enabled = 0},
		[6] = {.enabled = 1, .ppm=100, .base=1000},
		[7] = {},
		[8] = {.enabled = 1, .ppm=500, .base=10},
	},
	
	[8] = {
		[0] = {.enabled = 0,},
		[1] = {.enabled = 1, .ppm=300, .base=1},
		[2] = {.enabled = 1, .ppm=10000, .base=0},
		[3] = {.enabled = 0},
		[4] = {.enabled = 0},
		[5] = {.enabled = 0},
		[6] = {.enabled = 1, .ppm=100, .base=1000},
		[7] = {.enabled = 1, .ppm=500, .base=10},
		[8] = {},
	},
};


static void init_edges(struct dijkstra_edges *edges, size_t num_vertices)
{
	assert(num_vertices == NUM_VERTEXES);
	dijkstra_edges_init(edges, 1, num_vertices);
	
	for(uint32_t src_id = 0; src_id < NUM_VERTEXES; ++src_id)
	{
		for(uint32_t  dst_id = 0; dst_id < NUM_VERTEXES; ++dst_id) {
			struct ln_routing_fee *fee = &s_routing_fees[src_id][dst_id];
			if(fee->enabled <= 0) continue;
			
			struct dijkstra_sparse_edge * edge = edges->update(edges, src_id, dst_id, 0);
			assert(edge);
			edge->user_data = fee;
		}
	}
	
	
	return;
	
}

static int64_t calc_weight(int64_t amount, void * user_data)
{
	struct ln_routing_fee * fee = user_data;
	assert(fee);

	int64_t weight = amount * fee->ppm / MILLION + fee->base;
	return weight;
}

static int64_t calc_amount(int64_t amount, void * user_data)
{
	struct ln_routing_fee * fee = user_data;
	assert(fee);

	amount += amount * fee->ppm / MILLION + fee->base;
	return amount;
}

int main(int argc, char **argv)
{
	size_t num_vertices = NUM_VERTEXES;
	
	uint32_t src_id = 0;
	uint32_t dst_id = 8;
	int64_t amount = 100000;
	if(argc > 1) src_id = atoi(argv[1]);
	if(argc > 2) dst_id = atoi(argv[2]);
	if(argc > 3) amount = atol(argv[3]);
	
	
	struct dijkstra_vertex * vertices = NULL;
	struct dijkstra_edges edges[1];
	memset(edges, 0, sizeof(edges));
	init_edges(edges, num_vertices);

	struct dijkstra_graph graph = {
		.num_vertices = num_vertices,
		.vertices = vertices,
		.edges = edges,
	};
	struct dijkstra_context dijkstra[1];
	memset(dijkstra, 0, sizeof(dijkstra));
	
	dijkstra_context_init(dijkstra, &graph, NULL);
	dijkstra->calc_weight = calc_weight;
	dijkstra->calc_amount = calc_amount;
	struct clib_pointer_array candidates[1];
	memset(candidates, 0, sizeof(candidates));
	
	
	dijkstra->amount = amount;
	int64_t min_weight = dijkstra->shortest_path(dijkstra, src_id, dst_id, candidates);
	printf("min_weight=%ld\n", (long)min_weight);
	
	printf("path: count=%ld\n", candidates->length);
	for(size_t i = 0; i < candidates->length; ++i)
	{
		const struct dijkstra_vertex_status * status = candidates->data_ptrs[i];
		dijkstra_vertex_status_dump(status);
	}
	
	dijkstra_edges_cleanup(edges);
	clib_pointer_array_cleanup(candidates, NULL);
	dijkstra_context_cleanup(dijkstra);
	
	return 0;
}

