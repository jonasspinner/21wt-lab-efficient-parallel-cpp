#pragma once

#include <atomic>
#include <cassert>

#include "edge_list.hpp"
#include "adj_array.hpp"

class DynamicConnectivity {
 public:
    using Node = long;

    DynamicConnectivity() = default;
    explicit DynamicConnectivity(long num_nodes);

    // should contain #pragma omp parallel for
    void addEdges(const EdgeList& edges);

    [[nodiscard]] bool connected(Node a, Node b) const;

    // Must return -1 for roots.
    [[nodiscard]] Node parentOf(Node n) const;

private:
    std::vector<Node> union_find_parents;
    std::vector<std::size_t> union_find_ranks;

    EdgeList filtered_edges;

    std::vector<Node> bfs_frontier;
    std::vector<Node> bfs_next_frontier;
    std::vector<bool> bfs_visited;
    std::vector<Node> bfs_parents;


    [[nodiscard]] Node find_representative_and_compress(Node node);

    [[nodiscard]] Node find_representative(Node node) const;

    bool unite(Node a, Node b);

    void bfs(const AdjacencyArrayT<Node> &graph);
};
