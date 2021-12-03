#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_set>
#include <vector>

struct Hash {
    std::size_t operator()(const std::pair<long, long>& p) const {
        return p.first ^ (0x9e3779b9 + p.second + (p.first << 6) + (p.first >> 2));
    }
};

template <class R>
auto generateGraph(long num_nodes, double avg_degree, R& gen) {
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
                    edges.emplace(prev, next);
                else
                    edges.emplace(next, prev);
            }
            prev = next;
        }
    }

    // Generate remaining edges
    const long num_edges = avg_degree * num_nodes / 2;
    for (long left = num_edges - num_nodes + 1; left > 0; ) {
        auto a = dist(gen);
        auto b = dist(gen);
        if (a > b) std::swap(a, b);
        if (a == b || edges.find(std::make_pair(a, b)) != edges.end())
            continue;
        edges.emplace(a, b);
        --left;
    }

    std::vector<std::pair<long, long>> directed;
    directed.reserve(edges.size());

    {
        std::vector<std::vector<long>> vec(num_nodes);
        for (auto&& e : edges) {
            vec[e.first].push_back(e.second);
            vec[e.second].push_back(e.first);
        }

        std::vector<int> distance(num_nodes);
        std::queue<long> queue;
        queue.push(0);
        distance[0] = 1;
        while (!queue.empty()) {
            const auto n = queue.front();
            queue.pop();
            for (auto c : vec[n]) {
                if (!distance[c]) {
                    distance[c] = distance[n] + 1;
                    queue.push(c);
                }
            }
        }
        for (auto&& e : edges) {
            auto from = e.first;
            auto to = e.second;
            if (distance[to] < distance[from])
                std::swap(from, to);
            directed.emplace_back(from, to);
        }
    }

    return directed;
}

struct Node {
    unsigned long long work;
    std::vector<long> children;
};

inline void printStats(const std::vector<Node>& nodes) {
    std::vector<int> parent_count(nodes.size());
    std::vector<unsigned long long> parent_cost(nodes.size());
    unsigned long long critical = 0;
    int length = 0;

    for (auto&& n : nodes)
        for (auto&& c : n.children)
            ++parent_count[c];

    const auto dfs = [&](auto&& dfs, int i, int len) -> void {
        const auto& n = nodes[i];
        length = std::max(length, len + 1);
        if (n.children.empty()) {
            critical = std::max(critical, parent_cost[i] + n.work);
        } else {
            for (auto&& c : n.children) {
                parent_cost[c] += n.work;
                if (!--parent_count[c])
                    dfs(dfs, c, len + 1);
            }
        }
    };

    dfs(dfs, 0, 0);

    unsigned long long total = 0;
    for (auto&& n : nodes)
        total += n.work;

    std::cerr << "Total cost: " << total << "\n"
                 "Max path length: " << length << "\n"
                 "Critical path cost: " << critical
                 << " / " << (critical * 100 / total) << "%\n";

    const auto start = std::chrono::steady_clock::now();
    volatile unsigned long long x = 0;
    while (x < critical) ++x;
    const auto stop = std::chrono::steady_clock::now();

    std::cerr << "Critical path cost: "
              << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count()
              << " s\n";
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <#nodes> <avg_degree> <work_mean> <work_std> [seed]\n";
        return -1;
    }
    const auto num_nodes = std::atol(argv[1]);
    const auto avg_degree = std::atof(argv[2]);
    const auto work_mean = std::atof(argv[3]);
    const auto work_std = std::atof(argv[4]);
    if (num_nodes <= 0 || work_mean <= 0 || work_std < 0) {
        std::cerr << "Invalid argument.\n";
        return -1;
    }
    const unsigned long seed = argc > 5 ? std::atol(argv[5]) : num_nodes;

    std::vector<Node> nodes(num_nodes);
    auto gen = std::mt19937_64{seed};

    auto edges = generateGraph(num_nodes, avg_degree, gen);
    std::sort(edges.begin(), edges.end());
    for (auto&& e : edges) {
        assert(e.first < num_nodes);
        assert(e.second < num_nodes);
        nodes[e.first].children.push_back(e.second);
    }

    std::lognormal_distribution<double> dist(work_mean, work_std);
    for (auto& n : nodes)
        n.work = dist(gen);

    std::cout << num_nodes << ' ' << edges.size() << '\n';
    for (auto&& n : nodes) {
        std::cout << n.children.size();
        for (auto&& c : n.children)
            std::cout << ' ' << c;
        std::cout << ' ' << n.work << '\n';
    }

    printStats(nodes);

    return 0;
}
