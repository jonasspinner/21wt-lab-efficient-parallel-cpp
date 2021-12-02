#include <numeric>
#include <cassert>
#include <iostream>

#include "dynamic_connectivity.hpp"
#include "parallel_graph_algorithms.hpp"
#include "omp.h"

DynamicConnectivity::DynamicConnectivity(long num_nodes) : n(num_nodes)
{
    constexpr bool omp_init_arrays = true;

    union_find_parents = allocate_at_least<std::atomic<Node>>(n);
    union_find_ranks = allocate_at_least<std::atomic<Rank>>(n);

    filtered_edges = allocate_at_least<std::pair<Node, Node>>(n);

    adj_index = allocate_at_least<AdjIndex>(n + 1);
    adj_counter = allocate_at_least<std::atomic<AdjIndex>>(n + 1);
    adj_edges = allocate_at_least<Node>(2 * n);

    bfs_parents = allocate_at_least<std::atomic<Node>>(n);


    #pragma omp parallel default(none) shared(std::cout)
    {

        #pragma omp for
        for (Node u = 0; u < n; ++u) {
            ::new(&union_find_parents[u]) std::atomic<Node>(u);
        }

        #pragma omp for
        for (Node u = 0; u < n; ++u) {
            ::new(&union_find_ranks[u]) std::atomic<Rank>(0);
        }

        #pragma omp for
        for (Node u = 0; u < n; ++u) {
            ::new(&bfs_parents[u]) std::atomic<Node>(-1);
        }
    }

#ifndef NDEBUG
    for (Node u = 0; u < n; ++u) {
        assert(union_find_parents[u] == u);
        assert(union_find_ranks[u] == 0);
        assert(bfs_parents[u] == -1);
    }
#endif
}

DynamicConnectivity::~DynamicConnectivity() {
    free(union_find_parents);
    free(union_find_ranks);

    free(filtered_edges);
    num_filtered_edges.store(0);

    free(adj_index);
    free(adj_counter);
    free(adj_edges);

    free(bfs_parents);
}



void DynamicConnectivity::addEdges(const EdgeList& edges)
{
    std::size_t num_edges = edges.size();
    #pragma omp parallel for default(none), shared(edges, num_edges)
    for (std::size_t i = 0; i < num_edges; ++i) {
        Node a = static_cast<Node>(edges[i].from);
        Node b = static_cast<Node>(edges[i].to);

        assert(a < n && b < n);

        if (unite(a, b)) {
            std::size_t pos = num_filtered_edges++;
            ::new(&filtered_edges[pos]) std::pair<Node, Node>(a, b);
        }
    }


    parallel_build_adj_array(n, filtered_edges, num_filtered_edges.load(), adj_index, adj_counter, adj_edges);

    parallel_bfs_from_roots(n, union_find_parents, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers, bfs_visited, bfs_parents);
}



bool DynamicConnectivity::connected(Node a, Node b) const
{
    // Not thread safe
    return find_representative(a) == find_representative(b);
}


// Must return -1 for roots.
DynamicConnectivity::Node DynamicConnectivity::parentOf(Node node) const
{
    return bfs_parents[node].load();
}


DynamicConnectivity::Node DynamicConnectivity::find_representative(Node node) const
{
    // TODO: Make thread safe
    Node root = node;
    while (union_find_parents[root].load() != root) {
        root = union_find_parents[root].load();
    }
    return root;
}

DynamicConnectivity::Node DynamicConnectivity::find_representative_and_compress(Node node)
{
    // TODO: Make thread safe
    Node root = find_representative(node);

    while (union_find_parents[node].load() != root) {
        Node parent = union_find_parents[node].load();
        union_find_parents[node].store(root);
        node = parent;
    }

    return root;
}

bool DynamicConnectivity::unite(Node a, Node b) {
    // TODO: Make thread safe

#pragma omp critical
    {
        a = find_representative_and_compress(a);
        b = find_representative_and_compress(b);
    }

    /*
    a = find_representative(a);
    b = find_representative(b);
    */

    if (a == b) {
        return false;
    }

    Rank a_rank = union_find_ranks[a].load();
    Rank b_rank = union_find_ranks[b].load();
    if (a_rank < b_rank) {
        std::swap(a, b);
    }
    //union_find_parents[b].store(a);
    Node expected = b;
    if(union_find_parents[b].compare_exchange_strong(expected, a)) {
        if (a_rank == b_rank) {
            Rank expected_a_rank = a_rank;
            union_find_ranks[a].compare_exchange_strong(expected_a_rank, a_rank + 1);
        }
    }
    return true;
}