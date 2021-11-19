#pragma once

#include <cstddef>
#include <cassert>

#include "indexed_priority_queue.hpp"

// Construct your Dijkstra implementation here.  It should be used by first
// creating a DijkstraHelper with your graph, and then calling dijkstra on the helper
// (this should work for all graph implementations from tasks a, b, and c).
// You should also think, about the reason why we use this helper structure.

/*
  Example:
  auto nodeHelper = DijkstraHelper<NodeGraph>(nodeGraph);
  auto distance   = nodeHelper.dijkstra(handle1, handle2);
*/

template<class WeightedGraphClass>
class DijkstraHelper {
private:
    using GraphType = WeightedGraphClass;
    using NodeHandle = typename GraphType::NodeHandle;
    using EdgeIterator = typename GraphType::EdgeIterator;

    const GraphType &graph;

    std::vector<double> distance{};
    IndexedPriorityQueue<decltype(graph.nodeId(graph.node(0))), double, std::greater<>> queue;

    static_assert(std::is_same_v<decltype(graph.nodeId(graph.node(0))), std::size_t>);
public:
    explicit DijkstraHelper(const GraphType &graph) : graph(graph) {
        queue.reserve(graph.numNodes());
    }

    double dijkstra(NodeHandle start, NodeHandle end) {
        constexpr auto infty = std::numeric_limits<double>::infinity();

        if (start == end) {
            return 0.0;
        }

        auto start_id = graph.nodeId(start);

        queue.clear();
        queue.push(start_id, 0.0);

        distance.assign(graph.numNodes(), infty);
        distance[start_id] = 0.0;

        while (!queue.empty()) {
            auto [u_id, d_u] = queue.pop();
            auto u = graph.node(u_id);
            assert(d_u < infty);

            if (u == end) {
                return d_u;
            }

            for (auto e = graph.beginEdges(u); e < graph.endEdges(u); ++e) {
                auto v = graph.edgeHead(e);
                auto w = graph.edgeWeight(e);
                auto v_id = graph.nodeId(v);

                auto d_v = distance[v_id];
                auto d_v_from_u = d_u + w;

                if (d_v_from_u < d_v) {
                    queue.pushOrChangePriority(v_id, d_v_from_u);
                    distance[v_id] = d_v_from_u;
                }
            }
        }
        return infty;
    }

};
