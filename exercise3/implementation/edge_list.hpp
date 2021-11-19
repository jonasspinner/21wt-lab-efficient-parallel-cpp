#pragma once

#include <cstddef>
#include <fstream>
#include <utility>
#include <string>
#include <vector>

struct Edge {
    std::size_t from;
    std::size_t to;
    std::size_t length;
};

using EdgeList = std::vector<Edge>;

/**
 * Returns the list of edges and the number of nodes.
 */
inline std::pair<EdgeList, std::size_t> readEdges(const std::string& file) {
    std::pair<EdgeList, std::size_t> edges;
    std::ifstream in(file);
    in >> edges.second;
    std::size_t from, to, length;
    while (in >> from >> to >> length)
        edges.first.push_back({from, to, length});

    return edges;
}

