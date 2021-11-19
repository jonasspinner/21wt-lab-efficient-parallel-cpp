#pragma once

#include <cstddef>
#include <vector>
#include <cassert>
#include <memory>
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
class NodeGraphT {
 public:
    struct Node {
        Index id{0};
        std::vector<const Node*> neighbors{};
        explicit Node(Index id) : id(id) {}
    };

    using NodeHandle = const Node*;
    using EdgeIterator = typename std::vector<const Node*>::const_iterator;

    NodeGraphT() = default;
    NodeGraphT(std::size_t num_nodes, const EdgeList& edges) {
        nodes_.reserve(num_nodes);
        for (std::size_t id = 0; id < num_nodes; ++id) {
            nodes_.push_back(std::make_unique<Node>(id));
        }
        assert(nodes_.size() == num_nodes);
        for (const auto &e : edges) {
            NodeHandle n = nodes_[e.to].get();
            assert(n != nullptr);
            nodes_[e.from]->neighbors.push_back(n);
        }
    }

    [[nodiscard]] std::size_t numNodes() const {
        return nodes_.size();
    }

    [[nodiscard]] NodeHandle node(std::size_t id) const {
        assert(id < numNodes());
        return nodes_[id].get();
    }

    [[nodiscard]] std::size_t nodeId(NodeHandle n) const {
        assert(n != nullptr);
        return n->id;
    }

    [[nodiscard]] EdgeIterator beginEdges(NodeHandle n) const {
        return n->neighbors.begin();
    }
    [[nodiscard]] EdgeIterator endEdges(NodeHandle n) const {
        return n->neighbors.end();
    }

    [[nodiscard]] NodeHandle edgeHead(EdgeIterator e) const {
        return *e;
    }

    [[nodiscard]] double edgeWeight(EdgeIterator) const
    { return 1.; /* no weighted implementation */ }

 private:
    std::vector<std::unique_ptr<Node>> nodes_;
};

using NodeGraph = NodeGraphT<>;
