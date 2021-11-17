#pragma once

#include <atomic>
#include "edge_list.hpp"

class DynamicConnectivity {
 public:
    using Node = long;

    DynamicConnectivity();
    explicit DynamicConnectivity(long num_nodes);

    // should contain #pragma omp parallel for
    void addEdges(const EdgeList& edges);

    bool connected(Node a, Node b) const;

    // Must return -1 for roots.
    Node parentOf(Node n) const;

private:
    std::atomic<Node>* union_find;

    Node find_representative(Node a) const;
};
