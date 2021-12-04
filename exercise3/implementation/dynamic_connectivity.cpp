#include <numeric>
#include <cassert>
#include <iostream>

#include "dynamic_connectivity.hpp"
#include "graph_algorithms.hpp"
#include "omp.h"



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

    auto is_root = [&](Node u) { return is_rank_repr(union_find[u]); };

    parallel_build_adj_array(n, filtered_edges, num_filtered_edges.load(), adj_index, adj_counter, adj_edges);
    parallel_bfs_from_roots(n, is_root, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers, bfs_visited, bfs_parents);
}


DynamicConnectivity::Node DynamicConnectivity::find_representative(Node node) const
{
    assert(is_node_repr(node));
    Node p = union_find[node];
    if (is_rank_repr(p)) return node;
    return find_representative(p);
}

std::pair<DynamicConnectivity::Node, DynamicConnectivity::Node> DynamicConnectivity::find_representative_and_compress(Node node)
{
    assert(is_node_repr(node));
    Node p = union_find[node];
    if (is_rank_repr(p)) return {node, from_rank_repr(p)};
    auto [root, rank] = find_representative_and_compress(p);
    assert(rank >= 1);
    if (p != root)
        union_find[node].compare_exchange_strong(p, root);
    return {root, rank};
}

bool DynamicConnectivity::unite(Node a, Node b) {
    while (true) {
        Node rank_a, rank_b;
        std::tie(a, rank_a) = find_representative_and_compress(a);
        std::tie(b, rank_b) = find_representative_and_compress(b);
        Node rank_a_repr = to_rank_repr(rank_a);
        Node rank_b_repr = to_rank_repr(rank_b);
        //std::cerr << "a=" << a << " rank_a=" << rank_a << " A[a]=" << union_find[a]
        //          << " b=" << b << " rank_b=" << rank_b << " A[b]=" << union_find[b] << std::endl;
        assert(a >= 0 && b >= 0 && rank_a >= 1 && rank_b >= 1);
        if (a == b) return false;
        if (rank_a < rank_b) {
            if (union_find[a].compare_exchange_strong(rank_a_repr, b)) return true;
        } else if (rank_a > rank_b) {
            if (union_find[b].compare_exchange_strong(rank_b_repr, a)) return true;
        } else {
            if (a < b && union_find[a].compare_exchange_strong(rank_a_repr, b)) {
                union_find[b].compare_exchange_strong(rank_b_repr, to_rank_repr(rank_b + 1));
                return true;
            }
            if (a > b && union_find[b].compare_exchange_strong(rank_b_repr, a)) {
                union_find[a].compare_exchange_strong(rank_a_repr, to_rank_repr(rank_a + 1));
                return true;
            }
        }
    }
}