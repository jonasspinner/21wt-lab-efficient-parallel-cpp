#include <numeric>
#include <cassert>
#include <iostream>

#include "dynamic_connectivity_mt.hpp"
#include "graph_algorithms.hpp"
#include "omp.h"


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
        if (union_find_mutexes[node].try_lock()) {
            union_find_parents[node] = root;
            union_find_mutexes[node].unlock();
        }
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

        Rank a_rank = union_find_ranks[a];
        Rank b_rank = union_find_ranks[b];

        if (a_rank < b_rank) std::swap(a, b);
        union_find_parents[b] = a;
        if (a_rank == b_rank) {
            union_find_ranks[a]++;
        }

        if (a > b) std::swap(a, b);
        union_find_mutexes[b].unlock();
        union_find_mutexes[a].unlock();
        return true;
    }

    return false;
}