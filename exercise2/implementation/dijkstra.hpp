#pragma once

#include <cstddef>
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

    std::vector<NodeHandle> frontier{};
    std::vector<NodeHandle> next_frontier{};
    std::vector<double> distance{};
    IndexedPriorityQueue<std::size_t, double, std::greater<>> queue;

public:
    DijkstraHelper(const GraphType &graph) : graph(graph) {
        frontier.reserve(graph.numNodes());
        next_frontier.reserve(graph.numNodes());
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
            auto [u, d_u] = queue.pop();
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


    double dijkstra2(NodeHandle start, NodeHandle end) {
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
            auto [u, d_u] = queue.pop();
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

                if (queue.hasKey(v_id)) {
                    assert(d_v == queue.priority(v_id));
                }

                if (d_v == infty) {
                    queue.push(v_id, d_v_from_u);
                    distance[v_id] = d_v_from_u;
                } else if (d_v_from_u < d_v) {
                    queue.changePriority(v_id, d_v_from_u);
                    distance[v_id] = d_v_from_u;
                }

                /*
                if (d_v_from_u < d_v) {
                    queue.pushOrChangePriority(v_id, d_v_from_u);
                    distance[v_id] = d_v_from_u;
                }
                 */

                /*
                if (queue.hasKey(v_id)) {
                    auto d_v = queue.priority(v_id);
                    if (d_v_from_u < d_v) {
                        queue.changePriority(v_id, d_v_from_u);
                    }
                } else {
                    queue.push(v_id, d_v_from_u);
                }
                 */
            }
        }
        return infty;
    }

};
