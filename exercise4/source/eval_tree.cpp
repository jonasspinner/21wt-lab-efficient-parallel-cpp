
#include <iomanip>
#include "implementation/tree_solver.hpp"
#include "implementation/tree_solver_naive.hpp"
#include "utils/commandline.h"


void print_header_benchmark(std::ostream &os) {
    os
            << std::setw(25) << "\"file\"" << " "
            << std::setw(10) << "\"num_nodes\"" << " "
            << std::setw(10) << "\"capacity\"" << " "
            << std::setw(10) << "\"num_threads\"" << " "
            << std::setw(10) << "\"work_factor\"" << " "
            << std::setw(10) << "\"time (ms)\"" << " "
            << std::setw(10) << "\"success\""
            << std::endl;
}

void print_line_benchmark(
        std::ostream &os, const std::string &file, size_t num_nodes, size_t capacity, size_t num_threads,
        double work_factor, std::chrono::nanoseconds time, bool success) {
    os
            << std::setw(25) << file << " "
            << std::setw(10) << num_nodes << " "
            << std::setw(10) << capacity << " "
            << std::setw(10) << num_threads << " "
            << std::setw(10) << work_factor << " "
            << std::setw(10) << utils::to_ms(time) << " "
            << std::setw(10) << success
            << std::endl;
}

void benchmark_tree_solver(const std::string &out_file, const std::string &file, size_t num_work_factors,
                           size_t num_iterations,
                           size_t max_num_threads,
                           double min_work_factor,
                           double max_work_factor) {
    std::ofstream os(out_file);

    print_header_benchmark(os);
    print_header_benchmark(std::cout);

    for (size_t work_factor_idx = 0; work_factor_idx < num_work_factors; ++work_factor_idx) {
        double t = num_work_factors > 1 ? (double) work_factor_idx / ((double) num_work_factors - 1.0) : 0.0;
        double work_factor = min_work_factor * (1.0 - t) + max_work_factor * t;

        TreeTask tree(file, work_factor);

        size_t capacity = tree.size();

        for (size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
            TreeSolverNaive<TreeTask> solver(tree, capacity, num_threads);

            for (size_t i = 0; i < num_iterations; ++i) {
                auto t0 = std::chrono::high_resolution_clock::now();

                solver.solve();

                auto t1 = std::chrono::high_resolution_clock::now();

                print_line_benchmark(os, file, tree.size(), capacity, num_threads, work_factor, t1 - t0,
                                     tree.evaluate());
                print_line_benchmark(std::cout, file, tree.size(), capacity, num_threads, work_factor, t1 - t0,
                                     tree.evaluate());

                solver.reset();
            }
        }
    }
}


void print_header_stats(std::ostream &os, size_t num_threads) {
    os
            << std::setw(25) << "\"file\"" << " "
            << std::setw(10) << "\"num_nodes\"" << " "
            << std::setw(10) << "\"capacity\"" << " "
            << std::setw(10) << "\"num_threads\"" << " "
            << std::setw(10) << "\"work_factor\"" << " "
            << std::setw(10) << "\"time_point\"" << " "
            << std::setw(10) << "\"global\"";
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
        std::stringstream ss;
        ss << "\"thread " << (thread_id + 1) << "\"";
        os << " " << std::setw(10) << ss.str();
    }
    os << std::endl;
}

void print_line_stats(
        std::ostream &os, const std::string &file, size_t num_nodes, size_t capacity, size_t num_threads,
        double work_factor, std::chrono::high_resolution_clock::duration time_point, size_t global,
        std::vector<size_t> local) {
    os
            << std::setw(25) << file << " "
            << std::setw(10) << num_nodes << " "
            << std::setw(10) << capacity << " "
            << std::setw(10) << num_threads << " "
            << std::setw(10) << work_factor << " "
            << std::setw(10) << time_point.count() << " "
            << std::setw(10) << global;
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
        os << " " << std::setw(10) << local[thread_id];
    }
    os << std::endl;
}

void stats_tree_solver(
        const std::string &out_file, const std::string &file, size_t num_threads, double work_factor) {

    std::ofstream os(out_file);

    TreeTask tree(file, work_factor);

    auto capacity = tree.size();
    TreeSolver<TreeTask, true> solver(tree, capacity, num_threads);
    solver.solve();

    const auto &stats = solver.stats();


    print_header_stats(os, num_threads);
    print_header_stats(std::cout, num_threads);

    auto t0 = stats.m_time_points[0];
    for (size_t i = 0; i < stats.m_time_points.size(); ++i) {
        auto time_point = stats.m_time_points[i] - t0;
        auto global = stats.m_global_queue_size[i];
        std::vector<size_t> local;
        for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
            local.push_back(stats.m_local_queue_sizes[thread_id][i]);
        }

        print_line_stats(os, file, tree.size(), capacity, num_threads, work_factor, time_point, global, local);
        print_line_stats(std::cout, file, tree.size(), capacity, num_threads, work_factor, time_point, global, local);
    }
}

int main(int argn, char **argc) {
    CommandLine cl(argn, argc);

    std::string file = cl.strArg("-file", "../data/tree_100.graph");
    size_t num_work_factors = cl.intArg("-num-work-factors", 11);
    size_t num_iterations = cl.intArg("-num-iterations", 5);
    size_t max_num_threads = cl.intArg("-max-num-threads", (int) std::thread::hardware_concurrency());
    double min_work_factor = cl.doubleArg("-min-work-factor", 0.0);
    double max_work_factor = cl.doubleArg("-max-work-factor", 1.0);

    std::string type = cl.strArg("-type", "benchmark");

    if (type == "benchmark") {
    benchmark_tree_solver("../eval/eval_tree-graph_100.csv", file, num_work_factors, num_iterations, max_num_threads,
                          min_work_factor, max_work_factor);
    } else if (type == "stats") {
    //stats_tree_solver("../eval/tree_solver_queue_sizes_test.csv", file, max_num_threads, 0.1);
    }
    return 0;
}