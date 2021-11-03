#pragma once

#include <cstddef>

#include "edge_list.hpp"

class WeightedGraphSeparated {
 public:
    using NodeHandle = /* TODO */;
    using EdgeIterator = /* TODO */;

    WeightedGraphSeparated() = default;
    WeightedGraphSeparated(std::size_t num_nodes, const EdgeList& edges);

    std::size_t numNodes() const;

    NodeHandle node(std::size_t n) const;
    std::size_t nodeId(NodeHandle n) const;

    EdgeIterator beginEdges(NodeHandle) const;
    EdgeIterator endEdges(NodeHandle) const;

    NodeHandle edgeHead(EdgeIterator) const;
    double edgeWeight(EdgeIterator) const;

 private:
};
