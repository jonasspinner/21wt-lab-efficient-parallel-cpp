#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>

struct Hash {
    std::size_t operator()(const std::pair<long, long>& p) const {
        return p.first ^ (0x9e3779b9 + p.second + (p.first << 6) + (p.first >> 2));
    }
};

template <class R, class V>
void generateGraph(long start_id, long num_nodes, double avg_degree, R& gen, V& vec) {
    std::uniform_int_distribution<long> dist(0, num_nodes - 1);
    std::unordered_set<std::pair<long, long>, Hash> edges;

    {   // Generate random spanning tree
        std::vector<bool> seen(num_nodes, false);
        long prev = dist(gen);
        seen[prev] = true;
        long left = num_nodes - 1;
        while (left > 0) {
            const long next = dist(gen);
            if (!seen[next]) {
                seen[next] = true;
                --left;
                if (prev < next)
                    edges.emplace(start_id + prev, start_id + next);
                else
                    edges.emplace(start_id + next, start_id + prev);
            }
            prev = next;
        }
    }

    // Generate remaining edges
    const long num_edges = avg_degree * num_nodes / 2;
    for (long left = num_edges - num_nodes + 1; left > 0;) {
        auto a = start_id + dist(gen);
        auto b = start_id + dist(gen);
        if (a > b) std::swap(a, b);
        if (a == b || edges.find(std::make_pair(a, b)) != edges.end()) continue;
        edges.emplace(a, b);
        --left;
    }
    for (auto&& e : edges)
        vec.emplace_back(e);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <#components> <#nodes avg_degree> [#nodes avg_degree...]\n";
        return -1;
    }
    const auto num_components = std::atoi(argv[1]);
    if (num_components <= 0) {
        std::cerr << "Invalid argument.\n";
        return -1;
    }
    if (argc < 2 + 2 * num_components) {
        std::cerr << "Missing argument.\n";
        return -1;
    }

    std::vector<long> num_nodes;
    std::vector<double> avg_degrees;
    for (int i = 0, arg_idx = 2; i < num_components; ++i) {
        const auto nodes = std::atol(argv[arg_idx++]);
        const auto avg_degree = std::atof(argv[arg_idx++]);
        if (nodes <= 0 || avg_degree < 1) {
            std::cerr << "Invalid argument.\n";
            return -1;
        }
        num_nodes.push_back(nodes);
        avg_degrees.push_back(avg_degree);
    }
    const unsigned long total_nodes =
            std::accumulate(num_nodes.begin(), num_nodes.end(), 0);

    std::mt19937_64 gen{total_nodes};
    std::vector<std::pair<long, long>> edges;
    long id = 0;
    for (int i = 0; i < num_components; ++i) {
        generateGraph(id, num_nodes[i], avg_degrees[i], gen, edges);
        id += num_nodes[i];
    }

    // Shuffle nodes and edges
    std::vector<long> nodes(total_nodes);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::shuffle(nodes.begin(), nodes.end(), gen);
    std::shuffle(edges.begin(), edges.end(), gen);

    std::cout << total_nodes << '\n';
    for (auto&& e : edges)
        std::cout << nodes[e.first] << ' ' << nodes[e.second] << " 1\n";

    return 0;
}
