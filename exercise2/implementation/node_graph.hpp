#pragma once

#include <cstddef>
#include <vector>

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


class NodeGraph {
 public:
    struct Node {
        std::vector<Node*> neighbors;
    };

    using NodeHandle = /* TODO */;
    using EdgeIterator = /* TODO */;

    NodeGraph() = default;
    NodeGraph(std::size_t num_nodes, const EdgeList& edges);
    ~NodeGraph();

    std::size_t numNodes() const;

    NodeHandle node(std::size_t n) const;
    std::size_t nodeId(NodeHandle n) const;

    EdgeIterator beginEdges(NodeHandle) const;
    EdgeIterator endEdges(NodeHandle) const;

    NodeHandle edgeHead(EdgeIterator) const;
    double edgeWeight(EdgeIterator) const
    { return 1.; /* no weighted implementation */ }

 private:
    std::vector<Node*> nodes_;
};
