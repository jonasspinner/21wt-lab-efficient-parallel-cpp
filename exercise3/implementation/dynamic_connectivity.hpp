#pragma once

#include <atomic>
#include "edge_list.hpp"

class DynamicConnectivity {
 public:
    using Node = long;

    DynamicConnectivity() = default;

    explicit DynamicConnectivity(long num_nodes);

    ~DynamicConnectivity();

    // should contain #pragma omp parallel for
    void addEdges(const EdgeList& edges);

    [[nodiscard]] bool connected(Node a, Node b) const;

    // Must return -1 for roots.
    [[nodiscard]] Node parentOf(Node node) const;

private:
    using Rank = unsigned int;
    using AdjIndex = unsigned long;

    /**
     * Members
     */
    Node n{0};

    // union find
    std::atomic<Node>* union_find_parents{nullptr};
    std::atomic<Rank>* union_find_ranks{nullptr};

    // preliminary edge list
    std::pair<Node, Node>* filtered_edges{nullptr};
    std::atomic<std::size_t> num_filtered_edges{0};

    // preliminary graph
    AdjIndex* adj_index{nullptr};
    std::atomic<AdjIndex>* adj_counter{nullptr};
    Node* adj_edges{nullptr};

    // bfs
    std::vector<std::vector<Node>> bfs_frontiers;
    std::vector<std::vector<Node>> bfs_next_frontiers;
    std::vector<std::vector<bool>> bfs_visited;
    std::atomic<Node>* bfs_parents{nullptr};


    [[nodiscard]] Node find_representative(Node node) const;

    [[nodiscard]] Node find_representative_and_compress(Node node);

    bool unite(Node a, Node b);

    template<class T>
    static T* allocate_at_least(std::size_t size) {
        return static_cast<T *>(std::aligned_alloc(alignof(T), size * sizeof(T)));
    }
};
