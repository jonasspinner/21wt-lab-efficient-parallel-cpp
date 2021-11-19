#pragma once

#include <cstddef>
#include <vector>
#include <numeric>
#include <cstdint>

#include "edge_list.hpp"

// The interface is designed to iterate over all neighbors of a node v
// for (EdgeIterator it = graph.beginEdges(v); it != graph.endEdges(v); ++it) {
//     NodeHandle neighbor = graph.edgeHead(it);
//     // ... do something with the neighbor
// }
//
// You don't have to implement any iterators if you set the types correctly
// (i.e. using EdgeIterator= ...::iterator or ...::const_iterator)
// But if you want to implement an iterator you can find a blueprint in exc1


template<class Index = uint64_t>
class AdjacencyListT {
 public:
    using NodeHandle = Index;
    using EdgeIterator = typename std::vector<NodeHandle>::const_iterator;

    explicit AdjacencyListT(std::size_t num_nodes = 0, const EdgeList& edges = {}) : edges_(num_nodes) {
        if (num_nodes > static_cast<std::size_t>(std::numeric_limits<Index>::max())) {
            throw std::runtime_error("NodeIdType too small");
        }
        for (const auto &e : edges) {
            edges_[e.from].push_back(e.to);
        }
    }

    [[nodiscard]] std::size_t numNodes() const {
        return edges_.size();
    }

    [[nodiscard]] NodeHandle node(std::size_t n) const {
        return n;
    }

    [[nodiscard]] std::size_t nodeId(NodeHandle n) const {
        return n;
    }

    [[nodiscard]] EdgeIterator beginEdges(NodeHandle n) const {
        return edges_[n].begin();
    }

    [[nodiscard]] EdgeIterator endEdges(NodeHandle n) const {
        return edges_[n].end();
    }

    [[nodiscard]] NodeHandle edgeHead(EdgeIterator e) const {
        return *e;
    }

    [[nodiscard]] double edgeWeight(EdgeIterator) const
    { return 1.; /* no weighted implementation */ }

 private:
    std::vector<std::vector<NodeHandle>> edges_;
};

using AdjacencyList = AdjacencyListT<>;
