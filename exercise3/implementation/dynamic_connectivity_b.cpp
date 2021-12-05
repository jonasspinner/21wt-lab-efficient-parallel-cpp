#include <numeric>
#include <cassert>
#include <iostream>

#include "dynamic_connectivity_mt.hpp"
#include "graph_algorithms.hpp"
#include "omp.h"
#include "adj_array.hpp"


DynamicConnectivity::Node DynamicConnectivity::find_representative(Node node) const {
    Node root = union_find_parents[node];
    while (root != node) {
        node = root;
        root = union_find_parents[root];
    }
    return root;
}

bool DynamicConnectivity::unite(Node a, Node b) {
    a = find_representative(a);
    b = find_representative(b);

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
