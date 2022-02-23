#ifndef ALGORITHMS_C_DIJKSTRA_H_
#define ALGORITHMS_C_DIJKSTRA_H_

#include "algorithms-c-common.h"
#include <limits.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VERTEX_STATUS_MAX_CANDIDATES
#define VERTEX_STATUS_MAX_CANDIDATES (256)
#endif

#define DIJKSTRA_WEIGHT_UNSET (INT64_MAX)

struct dijkstra_vertex
{
	uint32_t id;	// index
	void * data;
};

struct dijkstra_sparse_edge
{
	int64_t weight;
	uint32_t src_id;	// src_vertex.id
	uint32_t dst_id;	// dst_vertex.id
	
	void * user_data;	// use to calc custom weigth, 
						// eg. struct routing_fees {int64_t rate, int64_t bias} fees = { ppm, base };
						// user_data := &fees;
						// calc_weight(amount, user_data) ==>  weight = amount * fees.rate + fees.bias;
};

struct dijkstra_edges
{
	int is_sparse_matrix;
	uint32_t num_vertices;
	union {
		struct {
			int64_t *weights;	// 2-d array
		};
		struct {
			void * search_root;	// binary tree search root
			struct clib_pointer_array vertex_edges_array[1]; // each row is a sorted-list which hold all the edges corresponding to each vertex, order by weights 
		};
	};
	
	// add or update an edge
	struct dijkstra_sparse_edge * (* update)(struct dijkstra_edges * edges, uint32_t src_id, uint32_t dst_id, int64_t weight);
	
	// remove an edge
	struct dijkstra_sparse_edge * (* remove)(struct dijkstra_edges * edges, uint32_t src_id, uint32_t dst_id);
	
	// get weight between two vertices
	int64_t (* get_weight)(struct dijkstra_edges * edges, uint32_t src_id, uint32_t dst_id);
	
	// get all edges belongs to a vertex
	ssize_t (* get_vertex_sparse_edges)(struct dijkstra_edges * edges, uint32_t vertex_id, const struct clib_slist ** p_edges);
};
struct dijkstra_edges * dijkstra_edges_init(struct dijkstra_edges * edges, int is_sparse_matrix, uint32_t num_vertices);
void dijkstra_edges_cleanup(struct dijkstra_edges *edges);

/************************************
 * dijkstra_graph
************************************/
struct dijkstra_graph
{
	size_t num_vertices;
	const struct dijkstra_vertex * vertices;
	const struct dijkstra_edges * edges;
};

/************************************
 * dijkstra_context
************************************/
struct dijkstra_vertex_status
{
	const struct dijkstra_vertex * vertex;
	int64_t min_weight;
	
	uint32_t id;
	int visited;
	int is_processing; // in working queue
	
	int depth;
	struct clib_pointer_array parent_candidates[1];
	
	int64_t amount; // custom data for calc weights
};
void dijkstra_vertex_status_dump(const struct dijkstra_vertex_status * status);

struct dijkstra_context
{
	void * user_data;
	const struct dijkstra_graph * graph;
	struct dijkstra_vertex_status * status_array;
	
	ssize_t (*shortest_path)(
		struct dijkstra_context * dijkstra, 
		uint32_t src_id, uint32_t dst_id,
		struct clib_pointer_array *first_candidates);
	
	
	int64_t amount;
	// custom callback to calc weight
	int64_t (* calc_weight)(int64_t amount, void * user_data);
	
	// custom callback to calc amount
	int64_t (* calc_amount)(int64_t amount, void * user_data);
};
struct dijkstra_context * dijkstra_context_init(
	struct dijkstra_context * dijkstra, 
	const struct dijkstra_graph * graph,
	void * user_data);
void dijkstra_context_cleanup(struct dijkstra_context * dijkstra);

#ifdef __cplusplus
}
#endif
#endif
