#pragma once

#include <cstdint>
#include <fstream>
#include <utility>
#include <string>
#include <vector>

struct Edge {
    std::uint64_t from;
    std::uint64_t to;
    double length;
};

using EdgeList = std::vector<Edge>;

/**
 * Returns the list of edges and the number of nodes.
 */
inline std::pair<EdgeList, std::size_t> readEdges(const std::string& file) {
    std::pair<EdgeList, std::size_t> edges;
    std::ifstream in(file);
    in >> edges.second;
    std::uint64_t from, to;
    double length;
    while (in >> from >> to >> length)
        edges.first.push_back({from, to, length});

    return edges;
}

