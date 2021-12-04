#include <numeric>
#include <cassert>
#include <iostream>

#include "dynamic_connectivity_mt.hpp"
#include "graph_algorithms.hpp"
#include "omp.h"


void DynamicConnectivity::addEdges(const EdgeList &edges) {
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

    auto is_root = [&](Node u) { return union_find_parents[u] == u; };

    sequential_build_adj_array(n, filtered_edges, num_filtered_edges.load(), adj_index, adj_counter, adj_edges);
    sequential_bfs_from_roots(n, is_root, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers,
                              bfs_visited, bfs_parents);
}


DynamicConnectivity::Node DynamicConnectivity::find_representative(Node node) const {
    Node root = union_find_parents[node];
    while (root != node) {
        node = root;
        root = union_find_parents[root];
    }
    return root;
}

DynamicConnectivity::Node DynamicConnectivity::find_representative_and_compress(Node node) {
    Node root = find_representative(node);

    while (union_find_parents[node] != root) {
        Node parent = union_find_parents[node];
        union_find_parents[node] = root;
        node = parent;
    }

    return root;
}

bool DynamicConnectivity::unite(Node a, Node b) {
    a = find_representative_and_compress(a);
    b = find_representative_and_compress(b);

    if (a == b) {
        return false;
    }

    if (try_lock_trees(a, b)) {
        assert(a < b);
        assert(union_find_mutexes[a].try_lock() == false);
        assert(union_find_mutexes[b].try_lock() == false);

        assert(union_find_parents[a] == a);
        assert(union_find_parents[b] == b);
        assert(union_find_parents[a] != union_find_parents[b]);

        union_find_parents[b] = a;

        union_find_mutexes[b].unlock();
        union_find_mutexes[a].unlock();
        return true;
    }

    return false;
}