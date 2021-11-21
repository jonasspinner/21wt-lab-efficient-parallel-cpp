#pragma once

#include <cstddef>
#include <numeric>
#include <cstdint>

#include "edge_list.hpp"


template<class Index = uint64_t>
class WeightedGraphPairedT {
public:
    using NodeHandle = Index;
    using EdgeIterator = typename std::vector<std::pair<NodeHandle, double>>::const_iterator;

    explicit WeightedGraphPairedT(std::size_t num_nodes = 0, const EdgeList &edges = {})
            : index_(num_nodes + 1), edges_(edges.size()) {
        std::vector<Index> c(num_nodes + 1);
        for (const auto &e: edges) {
            c[e.from + 1]++;
        }

        std::inclusive_scan(c.begin(), c.end(), c.begin());
        index_ = c;

        assert(c[0] == 0);
        assert(c[num_nodes] == edges.size());

        for (const auto &e: edges) {
            edges_[c[e.from]++] = {e.to, e.length};
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
        return edges_.begin() + index_[nodeId(n)];
    }

    [[nodiscard]] EdgeIterator endEdges(NodeHandle n) const {
        return edges_.begin() + index_[nodeId(n) + 1];
    }

    [[nodiscard]] NodeHandle edgeHead(EdgeIterator e) const {
        return e->first;
    }

    [[nodiscard]] double edgeWeight(EdgeIterator e) const {
        return e->second;
    }

private:
    std::vector<Index> index_;
    std::vector<std::pair<NodeHandle, double>> edges_;
};

using WeightedGraphPaired = WeightedGraphPairedT<>;
