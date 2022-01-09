
#include <iomanip>
#include "implementation/dag_solver.hpp"
#include "utils/commandline.h"


void print_header() {
    std::cout
            << std::setw(25) << "\"file\"" << " "
            << std::setw(10) << "\"num_nodes\"" << " "
            << std::setw(10) << "\"capacity\"" << " "
            << std::setw(10) << "\"num_threads\"" << " "
            << std::setw(10) << "\"work_factor\"" << " "
            << std::setw(10) << "\"time (ms)\"" << " "
            << std::setw(10) << "\"success\""
            << std::endl;
}

void print_line(const std::string &file, size_t num_nodes, size_t capacity, size_t num_threads, double work_factor,
                std::chrono::nanoseconds time, bool success) {
    std::cout
            << std::setw(25) << file << " "
            << std::setw(10) << num_nodes << " "
            << std::setw(10) << capacity << " "
            << std::setw(10) << num_threads << " "
            << std::setw(10) << work_factor << " "
            << std::setw(10) << utils::to_ms(time) << " "
            << std::setw(10) << success
            << std::endl;
}

int main(int argn, char **argc) {
    CommandLine cl(argn, argc);

    std::string file = cl.strArg("-file", "../data/graph_100.graph");
    size_t num_work_factors = cl.intArg("-num-work-factors", 11);
    size_t num_iterations = cl.intArg("-num-iterations", 5);
    size_t max_num_threads = cl.intArg("-max-num-threads", (int) std::thread::hardware_concurrency());
    double min_work_factor = cl.doubleArg("-min-work-factor", 0.0);
    double max_work_factor = cl.doubleArg("-max-work-factor", 1.0);

    print_header();

    for (size_t work_factor_idx = 0; work_factor_idx < num_work_factors; ++work_factor_idx) {
        double t = num_work_factors > 1 ? (double) work_factor_idx / ((double) num_work_factors - 1.0) : 0.0;
        double work_factor = min_work_factor * (1.0 - t) + max_work_factor * t;

        DAGTask dag(file, work_factor);

        size_t capacity = dag.size();

        for (size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
            DAGSolver<DAGTask> solver(dag, capacity, num_threads);

            for (size_t i = 0; i < num_iterations; ++i) {
                auto t0 = std::chrono::high_resolution_clock::now();

                solver.solve();

                auto t1 = std::chrono::high_resolution_clock::now();

                print_line(file, dag.size(), capacity, num_threads, work_factor, t1 - t0, dag.evaluate());

                solver.reset();
            }
        }
    }


    return 0;
}