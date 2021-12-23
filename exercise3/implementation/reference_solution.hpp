#pragma once

#include "edge_list.hpp"

#include <atomic>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

class ReferenceSolution {
 public:
    using Node = std::uint32_t;

    static constexpr Node no_parent = static_cast<Node>(-1);

    struct Edge {
        Node from;
        Node to;
    };

    ReferenceSolution() = default;
    explicit ReferenceSolution(std::size_t num_nodes);

    void addEdges(const EdgeList& edges);

    std::size_t numNodes() const { return parent_of_.size(); }
    std::size_t numEdges() const { return edges_.size(); }

    const std::vector<Edge>& edges() const { return edges_; }

    bool connected(Node a, Node b) const {
        return find_representative(a).root == find_representative(b).root;
    }

    std::make_signed_t<Node> parentOf(Node n) const {
        return static_cast<std::make_signed_t<Node>>(parent_of_[n]);
    }

private:
    using NodeOrRank = std::make_signed_t<Node>;
    using EdgeIndex = Node;

    struct RootAndRank {
        Node root;
        NodeOrRank rank;
    };

    class Graph {
     public:
        void build(const ReferenceSolution& dc);

        template <class F>
        void for_each_neighbor_of(Node node, F&& func) const;
     private:
        std::vector<std::atomic<EdgeIndex>> index_;
        std::vector<Node> edges_;
    };

    mutable std::vector<std::atomic<NodeOrRank>> union_find_;
    std::vector<Node> parent_of_;
    std::vector<Edge> edges_;
    Graph graph_;

    RootAndRank find_representative(Node a) const;
    bool unite(Edge edge);

    bool link(RootAndRank parent, RootAndRank child);
    void increase_rank(RootAndRank root);

    void rebuild_parent_array();
};
