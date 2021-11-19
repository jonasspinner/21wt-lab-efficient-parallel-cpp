#include <chrono>
#include <iostream>
#include <random>

#include "../implementation/node_graph.hpp"
#include "../implementation/adj_array.hpp"
#include "../implementation/adj_list.hpp"
#include "../implementation/bfs.hpp"

template<class T>
void print(std::ostream &out, const T& v, std::streamsize w) {
    out.width(w);
    out << v << " ";
    std::cout.width(w);
    std::cout << v << " ";
}



double mean(const std::vector<double> &x) {
    auto sum = std::accumulate(x.begin(), x.end(), 0.0);
    return sum / x.size();
}

double standard_deviation(const std::vector<double> &x) {
    auto m = mean(x);
    double sum = 0.0;
    for (auto x_i : x) {
        sum += (x_i - m) * (x_i - m);
    }
    return std::sqrt(sum / (x.size() - 1));
}

void print_header_construction(std::ostream &out) {
    print(out, "\"graph class name\"", 20);
    print(out, "\"graph instance name\"", 20);
    print(out, "\"n\"", 8);
    print(out, "\"m\"", 8);
    print(out, "\"graph constructor (ms)\"", 8);
    print(out, "\"bfs constructor (ms)\"", 8);
    print(out, "\"number of queries\"", 8);
    print(out, "\"bfs total (ms)\"", 8);
    print(out, "\"bfs mean (ms)\"", 8);
    print(out, "\"bfs std (ms)\"", 8);
    print(out, "\"total (ms)\"", 8);
    out << "\n";
    std::cout << "\n";
}

template<class GraphClass>
void run_benchmark_construction(std::ostream &out, std::string_view graph_class_name, std::string_view graph_instance_name,
                   std::size_t num_nodes, const EdgeList &edges, const std::vector<std::pair<std::size_t, std::size_t>> &queries) {
    auto t0 = std::chrono::high_resolution_clock::now();

    GraphClass graph(num_nodes, edges);

    auto t1 = std::chrono::high_resolution_clock::now();

    BFSHelper<GraphClass> bfs(graph);

    auto t2 = std::chrono::high_resolution_clock::now();

    std::vector<double> bfs_times;
    for (auto [start, end] : queries) {
        auto t_bfs_start = std::chrono::high_resolution_clock::now();

        [[maybe_unused]] auto dist = bfs.bfs(graph.node(start), graph.node(end));

        auto t_bfs_end = std::chrono::high_resolution_clock::now();
        bfs_times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t_bfs_end - t_bfs_start).count() / 1000.);
    }

    auto t3 = std::chrono::high_resolution_clock::now();

    auto construct_graph = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.;
    auto construct_bfs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000.;
    auto run_bfs_total = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() / 1000.;
    auto total = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t0).count() / 1000.;

    print(out, graph_class_name, 20);
    print(out, graph_instance_name, 20);
    print(out, num_nodes, 8);
    print(out, edges.size(), 8);
    print(out, construct_graph, 8);
    print(out, construct_bfs, 8);
    print(out, queries.size(), 8);
    print(out, run_bfs_total, 8);
    print(out, mean(bfs_times), 8);
    print(out, standard_deviation(bfs_times), 8);
    print(out, total, 8);
    out << "\n";
    std::cout << "\n";
}


void print_header_runs(std::ostream &out) {
    print(out, "\"graph class name\"", 20);
    print(out, "\"graph instance name\"", 20);
    print(out, "\"n\"", 8);
    print(out, "\"m\"", 8);
    print(out, "\"distance\"", 8);
    print(out, "\"bfs (ms)\"", 8);
    out << "\n";
    std::cout << "\n";
}

template<class GraphClass>
void run_benchmark_runs(std::ostream &out, std::string_view graph_class_name, std::string_view graph_instance_name,
                                std::size_t num_nodes, const EdgeList &edges, const std::vector<std::pair<std::size_t, std::size_t>> &queries) {
    GraphClass graph(num_nodes, edges);
    BFSHelper<GraphClass> bfs(graph);

    for (auto [start, end] : queries) {
        auto t_bfs_start = std::chrono::high_resolution_clock::now();

        auto dist = bfs.bfs(graph.node(start), graph.node(end));

        auto t_bfs_end = std::chrono::high_resolution_clock::now();
        auto t = std::chrono::duration_cast<std::chrono::microseconds>(t_bfs_end - t_bfs_start).count() / 1000.;

        print(out, graph_class_name, 20);
        print(out, graph_instance_name, 20);
        print(out, num_nodes, 8);
        print(out, edges.size(), 8);
        print(out, dist, 8);
        print(out, t, 8);
        out << "\n";
        std::cout << "\n";
    }
}

std::vector<std::pair<std::size_t, std::size_t>> generate_uniform_random_queries(std::size_t num_nodes, std::size_t num_queries) {
    std::mt19937_64 gen;
    std::uniform_int_distribution<std::size_t> node_dist(0, num_nodes - 1);
    std::vector<std::pair<std::size_t, std::size_t>> queries;
    for (std::size_t i = 0; i < num_queries; ++i) {
        queries.emplace_back(node_dist(gen), node_dist(gen));
    }
    return queries;
}

std::vector<std::pair<std::size_t, std::size_t>> generate_uniform_random_query(std::size_t num_nodes, std::size_t num_queries) {
    std::mt19937_64 gen;
    std::uniform_int_distribution<std::size_t> node_dist(0, num_nodes - 1);
    auto start = node_dist(gen);
    auto end = node_dist(gen);
    std::vector<std::pair<std::size_t, std::size_t>> queries;
    for (std::size_t i = 0; i < num_queries; ++i) {
        queries.emplace_back(start, end);
    }
    return queries;
}

int main() {
    const std::vector<std::string> graph_files = {"../data/netherlands.graph"};
    const auto [edges, num_nodes] = readEdges(graph_files[0]);

    std::size_t num_queries = 20;

    auto queries = generate_uniform_random_queries(num_nodes, num_queries);

    auto file_construction = std::ofstream("benchmark-construction.csv");
    print_header_construction(file_construction);
    run_benchmark_construction<NodeGraphT<uint32_t>>(file_construction, "NodeGraph<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_construction<NodeGraphT<uint64_t>>(file_construction, "NodeGraph<u64>", "netherlands", num_nodes, edges, queries);
    run_benchmark_construction<AdjacencyListT<uint32_t>>(file_construction, "AdjacencyList<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_construction<AdjacencyListT<uint64_t>>(file_construction, "AdjacencyList<u64>", "netherlands", num_nodes, edges, queries);
    run_benchmark_construction<AdjacencyArrayT<uint32_t>>(file_construction, "AdjacencyArray<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_construction<AdjacencyArrayT<uint64_t>>(file_construction, "AdjacencyArray<u64>", "netherlands", num_nodes, edges, queries);

    auto file_runs = std::ofstream("benchmark-runs.csv");
    print_header_runs(file_runs);
    run_benchmark_runs<NodeGraphT<uint32_t>>(file_runs, "NodeGraph<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_runs<NodeGraphT<uint64_t>>(file_runs, "NodeGraph<u64>", "netherlands", num_nodes, edges, queries);
    run_benchmark_runs<AdjacencyListT<uint32_t>>(file_runs, "AdjacencyList<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_runs<AdjacencyListT<uint64_t>>(file_runs, "AdjacencyList<u64>", "netherlands", num_nodes, edges, queries);
    run_benchmark_runs<AdjacencyArrayT<uint32_t>>(file_runs, "AdjacencyArray<u32>", "netherlands", num_nodes, edges, queries);
    run_benchmark_runs<AdjacencyArrayT<uint64_t>>(file_runs, "AdjacencyArray<u64>", "netherlands", num_nodes, edges, queries);


    return 0;
}