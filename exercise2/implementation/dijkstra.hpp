#pragma once

#include <cstddef>

// Construct your Dijkstra implementation here.  It should be used by first
// creating a DijkstraHelper with your graph, and then calling dijkstra on the helper
// (this should work for all graph implementations from tasks a, b, and c).
// You should also think, about the reason why we use this helper structure.

/*
  Example:
  auto nodeHelper = DijkstraHelper<NodeGraph>(nodeGraph);
  auto distance   = nodeHelper.dijkstra(handle1, handle2);
*/

template <class WeightedGraphClass>
class DijkstraHelper
{
private:
    using GraphType    = WeightedGraphClass;
    using NodeHandle   = typename GraphType::NodeHandle;
    using EdgeIterator = typename GraphType::EdgeIterator;

    const GraphType& graph;

    // TODO maybe other data members

public:
    DijkstraHelper(const GraphType& graph) : graph(graph) { }

    double dijkstra(NodeHandle start, NodeHandle end)
    {
        // TODO implementation
    }

};
