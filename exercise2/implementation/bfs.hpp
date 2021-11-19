#pragma once

#include <cstddef>

// Construct your BFS implementation here.  It should be used by first creating
// a BFSHelper with your graph, and then calling bfs on the helper (this should
// work for all graph implementations from tasks a, b, and c).
// You should also think, about the reason why we use this helper structure.

/*
  Example:
  auto nodeHelper = BFSHelper<NodeGraph>(nodeGraph);
  auto distance   = nodeHelper.bfs(handle1, handle2);
*/

template<class GraphClass>
class BFSHelper {
private:
    using GraphType = GraphClass;
    using NodeHandle = typename GraphType::NodeHandle;
    using EdgeIterator = typename GraphType::EdgeIterator;

    const GraphType &graph;

    std::vector<NodeHandle> frontier{};
    std::vector<NodeHandle> next_frontier{};
    std::vector<bool> visited{};

public:
    explicit BFSHelper(const GraphType &graph) : graph(graph) {
        frontier.reserve(graph.numNodes());
        next_frontier.reserve(graph.numNodes());
        visited.reserve(graph.numNodes());
    }

    std::size_t bfs(NodeHandle start, NodeHandle end) {
        if (start == end) {
            return 0;
        }

        frontier.clear();
        frontier.push_back(start);
        next_frontier.clear();
        visited.assign(graph.numNodes(), false);

        visited[graph.nodeId(start)] = true;

        std::size_t distance = 1;

        while (!frontier.empty()) {
            while (!frontier.empty()) {
                NodeHandle n = std::move(frontier.back());
                frontier.pop_back();

                for (EdgeIterator e = graph.beginEdges(n); e < graph.endEdges(n); ++e) {
                    NodeHandle neighbor = graph.edgeHead(e);
                    if (neighbor == end) {
                        return distance;
                    }
                    auto id = graph.nodeId(neighbor);
                    if (!visited[id]) {
                        visited[id] = true;
                        next_frontier.push_back(neighbor);
                    }
                }
            }
            distance++;
            std::swap(frontier, next_frontier);
        }
        return std::numeric_limits<std::size_t>::max();
    }

};
