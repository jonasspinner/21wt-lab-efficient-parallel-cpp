#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "omp.h"

#ifdef DC_SEQUENTIAL
    #include "implementation/dynamic_connectivity_sequential.hpp"
#elif DC_LOCKLESS
    #include "implementation/dynamic_connectivity.hpp"
#endif

#include "implementation/edge_list.hpp"
#include "utils/commandline.hpp"

int main(int argn, char** argc) {
    //omp_set_num_threads(1024);

    CommandLine c(argn, argc);

    std::string graph      = c.strArg("-graph", "../data/test_graph1.graph");
    std::string test_graph = c.strArg("-test" , "../data/test_correctness1.graph");

    EdgeList edges;
    EdgeList to_check;

    std::size_t num_nodes;
    std::size_t num_blocks;

    std::tie(edges, num_nodes)     = readEdges(graph);
    std::tie(to_check, num_blocks) = readEdges(test_graph);

    const auto block_size = edges.size() / num_blocks;

    DynamicConnectivity dc(num_nodes);

    std::vector<bool> seen;
    std::vector<long> path;
    const auto isForest = [&, num_nodes = num_nodes] {
        seen.clear();
        seen.resize(num_nodes, false);
        for (long start = 0; start < long(num_nodes); ++start) {
            path.clear();
            long n = start;
            while (n != -1 && !seen[n]) {
                if (std::find(path.begin(), path.end(), n) != path.end())
                    return false;
                path.push_back(n);
                seen[n] = true;
                n = dc.parentOf(n);
            }
        }
        return true;
    };

    for (std::size_t block = 0; block < num_blocks; ++block) {
        const EdgeList block_edges(edges.begin() + block * block_size,
                                   edges.begin() + (block + 1) * block_size);
        dc.addEdges(block_edges);

        if (!isForest())
            throw std::runtime_error{"Datastructure is not a forest."};

        for (const auto& p : to_check) {
            const bool connected = dc.connected(p.from, p.to);
            if (connected != (p.length <= block))
                throw std::runtime_error{std::to_string(p.from) + " and "
                                         + std::to_string(p.to) + " expected to be in "
                                         + std::string(connected ? "different components"
                                                                 : "the same component")
                                         + " after block " + std::to_string(block) + '.'};
        }
    }
    std::cout << "correctness test passed\n";
    return 0;
}
