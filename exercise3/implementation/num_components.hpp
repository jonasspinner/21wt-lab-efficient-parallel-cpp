#pragma once

#include "edge_list.hpp"

class ComponentsCounter {
 public:
    using Node = long;

    ComponentsCounter();
    explicit ComponentsCounter(long num_nodes);
    std::size_t addEdges(const EdgeList& edges);

private:
    std::vector<Node> union_find;
    std::size_t num_components;
    Node find_representative(Node a) const;
};

ComponentsCounter::ComponentsCounter(long num_nodes)
    : union_find(num_nodes, -1), num_components(num_nodes)
{ }

std::size_t ComponentsCounter::addEdges(const EdgeList& edges)
{
    for (auto e : edges)
    {
        auto rep_from = find_representative(e.from);
        auto rep_to   = find_representative(e.to);
        if (rep_from == rep_to)
            continue;
        if (rep_from > rep_to)
            std::swap(rep_from, rep_to); // union by ID
        union_find[rep_from] = rep_to;
        num_components--;
    }
    return num_components;
}

ComponentsCounter::Node ComponentsCounter::find_representative(Node a) const
{
    auto rep = a;
    while (union_find[rep] >= 0)
    {
        rep = union_find[rep];
    }
    return rep;
}
