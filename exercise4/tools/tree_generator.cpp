#include <algorithm>
#include <chrono>
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

template <class R>
auto generateTree(long num_nodes, R&& gen) {
    std::uniform_int_distribution<long> dist(0, num_nodes - 1);
    std::unordered_set<std::pair<long, long>, Hash> edges;

    {   // Generate random spanning tree
        std::vector<bool> seen(num_nodes, false);
        long prev = 0;
        seen[0] = true;
        long left = num_nodes - 1;
        while (left > 0) {
            const long next = dist(gen);
            if (!seen[next]) {
                seen[next] = true;
                --left;
                edges.emplace(prev, next);
            }
            prev = next;
        }
    }

    return std::vector<std::pair<long, long>>(edges.begin(), edges.end());
}

struct Node {
    unsigned long long work;
    std::vector<long> children;
};

inline void printStats(const std::vector<Node>& nodes) {
    unsigned long long total = 0;
    int length = 0;
    unsigned long long critical = 0;

    const auto dfs = [&](auto&& dfs, int i, int len, unsigned long long sum) -> void {
        const auto& n = nodes[i];
        total += n.work;
        if (n.children.empty()) {
            length = std::max(length, len + 1);
            critical = std::max(critical, sum + n.work);
        } else {
            for (auto&& c : n.children)
                dfs(dfs, c, len + 1, sum + n.work);
        }
    };

    dfs(dfs, 0, 0, 0);

    std::cerr << "Total cost: " << total << "\n"
                 "Max path length: " << length << "\n"
                 "Critical path cost: " << critical <<
                 " / " << (critical * 100 / total) << "%\n";

    const auto start = std::chrono::steady_clock::now();
    volatile unsigned long long x = 0;
    while (x < critical) ++x;
    const auto stop = std::chrono::steady_clock::now();

    std::cerr << "Critical path cost: "
              << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count()
              << " s\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <#nodes> <work_mean> <work_std> [seed]\n";
        return -1;
    }
    const auto num_nodes = std::atol(argv[1]);
    const auto work_mean = std::atof(argv[2]);
    const auto work_std = std::atof(argv[3]);
    if (num_nodes <= 0 || work_mean <= 0 || work_std < 0) {
        std::cerr << "Invalid argument.\n";
        return -1;
    }
    const unsigned long seed = argc > 4 ? std::atol(argv[4]) : num_nodes;

    std::vector<Node> nodes(num_nodes);
    auto gen = std::mt19937_64{seed};

    auto edges = generateTree(num_nodes, gen);
    std::sort(edges.begin(), edges.end());
    for (auto&& e : edges)
        nodes[e.first].children.push_back(e.second);

    std::lognormal_distribution<double> dist(work_mean, work_std);
    for (auto& n : nodes)
        n.work = dist(gen);

    std::cout << num_nodes << '\n';
    for (auto&& n : nodes) {
        std::cout << n.children.size();
        for (auto&& c : n.children)
            std::cout << ' ' << c;
        std::cout << ' ' << n.work << '\n';
    }

    printStats(nodes);

    return 0;
}
