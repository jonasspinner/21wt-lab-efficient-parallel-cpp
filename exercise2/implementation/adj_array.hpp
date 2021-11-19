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
class AdjacencyArrayT {
public:
    using NodeHandle = Index;
    using EdgeIterator = Index;

    explicit AdjacencyArrayT(std::size_t num_nodes = 0, const EdgeList &edges = {})
            : index_(num_nodes + 1), edges_(edges.size()) {
        if (num_nodes > static_cast<std::size_t>(std::numeric_limits<Index>::max())) {
            throw std::runtime_error("NodeIdType too small");
        }

        std::vector<Index> c(num_nodes + 1);
        for (const auto &e: edges) {
            c[e.from + 1]++;
        }

        std::inclusive_scan(c.begin(), c.end(), c.begin());
        index_ = c;

        assert(c[0] == 0);
        assert(c[num_nodes] == edges.size());

        for (const auto &e: edges) {
            edges_[c[e.from]++] = e.to;
        }
    }

    [[nodiscard]] std::size_t numNodes() const {
        return index_.size() - 1;
    };

    [[nodiscard]] NodeHandle node(std::size_t n) const {
        return n;
    }

    [[nodiscard]] std::size_t nodeId(NodeHandle n) const {
        return n;
    }

    [[nodiscard]] EdgeIterator beginEdges(NodeHandle n) const {
        return index_[nodeId(n)];
    }

    [[nodiscard]] EdgeIterator endEdges(NodeHandle n) const {
        return index_[nodeId(n) + 1];
    }

    [[nodiscard]] NodeHandle edgeHead(EdgeIterator e) const {
        return edges_[e];
    }

    [[nodiscard]] double edgeWeight(EdgeIterator) const { return 1.; /* no weighted implementation */ }

private:
    std::vector<Index> index_;
    std::vector<NodeHandle> edges_;
};

using AdjacencyArray = AdjacencyArrayT<>;
