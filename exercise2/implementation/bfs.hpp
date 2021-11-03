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

template <class GraphClass>
class BFSHelper
{
private:
    using GraphType    = GraphClass;
    using NodeHandle   = typename GraphType::NodeHandle;
    using EdgeIterator = typename GraphType::EdgeIterator;

    const GraphType& graph;

    // TODO maybe other data members

public:
    BFSHelper(const GraphType& graph) : graph(graph) { }

    std::size_t bfs(NodeHandle start, NodeHandle end)
    {
        // TODO implementation
    }

};
