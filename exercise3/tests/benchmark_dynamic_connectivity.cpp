#include <vector>
#include <mutex>
#include <iostream>
#include <iomanip>

#include "omp.h"

#if defined(DC_SEQUENTIAL)
#include "implementation/dynamic_connectivity_sequential.hpp"
#elif defined(DC_B) || defined(DC_C) || defined(DC_D) || defined(DC_E)
#include "implementation/dynamic_connectivity_mt.hpp"
#elif defined(DC_F)
#include "implementation/dynamic_connectivity.hpp"
#endif

#include "utils/commandline.hpp"
#include "implementation/edge_list.hpp"

int main(int argn, char** argc) {
    CommandLine c(argn, argc);

    int num_threads = c.intArg("-num-threads", -1);
    std::string graph_path = c.strArg("-graph", "../data/10x-1e6-2.graph");

    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    } else {
        num_threads = omp_get_max_threads();
    }

    auto [edges, num_nodes] = readEdges(graph_path);

    std::cout << "num_nodes   = " << std::setw(10) << num_nodes << "\n";
    std::cout << "num_edges   = " << std::setw(10) << edges.size() << "\n";
    std::cout << "num_threads = " << std::setw(10) << num_threads << "\n";

    DynamicConnectivity dc(num_nodes);

    auto t0 = std::chrono::high_resolution_clock::now();
    dc.addEdges(edges);
    auto t1 = std::chrono::high_resolution_clock::now();

    std::cout << "time        = " << std::setw(10) << ((double)(t1 - t0).count() / 1'000'000.0 ) << "\n";

    return 0;
}