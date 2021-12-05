#include <mutex>
#include <iostream>
#include <iomanip>

#include "omp.h"

#include "implementation/dynamic_connectivity.hpp"


#include "utils/commandline.hpp"
#include "implementation/edge_list.hpp"

constexpr std::string_view task() {
#if defined(DC_SEQUENTIAL)
    return "a";
#elif defined(DC_B)
    return "b";
#elif defined(DC_C)
    return "c";
#elif defined(DC_D)
    return "d";
#elif defined(DC_E)
    return "e";
#elif defined(DC_F)
    return "f";
#else
    static_assert(false);
#endif
}


double milliseconds(std::chrono::nanoseconds time) {
    return ((double)time.count() / 1'000'000.0 );
}

int main(int argn, char** argc) {
    CommandLine c(argn, argc);

    std::string graph_path = c.strArg("-graph", "../data/10x-1e6-2.graph");
    int num_threads = c.intArg("-num-threads", -1);
    bool thread_range = c.boolArg("-thread-range");
    int num_iterations = c.intArg("-num-iterations", 1);
    bool no_header = c.boolArg("-no-header");

    if (!c.report()) return EXIT_FAILURE;

    if (!no_header) {
        std::cout << std::setw(5) << "task" << " ";
        std::cout << std::setw(30) << "graph" << " ";
        std::cout << std::setw(10) << "num_nodes" << " ";
        std::cout << std::setw(10) << "num_edges" << " ";
        std::cout << std::setw(10) << "num_threads" << " ";
        std::cout << std::setw(10) << "construction_time" << " ";
        std::cout << std::setw(10) << "time" << std::endl;
    }

    auto [edges, num_nodes] = readEdges(graph_path);

    if (num_threads == -1) {
        num_threads = omp_get_max_threads();
    }
    int max_num_threads = num_threads;
    if (thread_range) {
        num_threads = 1;
    } else {
        max_num_threads = num_threads;
    }

    for (; num_threads <= max_num_threads; ++num_threads) {
        omp_set_num_threads(num_threads);
#pragma omp parallel default(none) shared(num_threads)
        {
#pragma omp single
            {
                if (num_threads != omp_get_num_threads())
                    throw std::runtime_error("not the correct number of threads");
            }
        }

        for (int it = 0; it < num_iterations; ++it) {
            auto t0 = std::chrono::high_resolution_clock::now();

            DynamicConnectivity dc(num_nodes);

            auto t1 = std::chrono::high_resolution_clock::now();

            dc.addEdges(edges);

            auto t2 = std::chrono::high_resolution_clock::now();

            std::cout << std::setw(5) << task() << " ";
            std::cout << std::setw(30) << graph_path << " ";
            std::cout << std::setw(10) << num_nodes << " ";
            std::cout << std::setw(10) << edges.size() << " ";
            std::cout << std::setw(10) << num_threads << " ";
            std::cout << std::setw(10) << milliseconds(t1 - t0) << " ";
            std::cout << std::setw(10) << milliseconds(t2 - t1) << std::endl;
        }
    }
    return 0;
}