#include <chrono>
#include <iostream>
#include <random>
#include <sstream>

#include "../implementation/node_graph.hpp"
#include "../implementation/adj_array.hpp"
#include "../implementation/adj_list.hpp"
#include "../implementation/weighted_graph_paired.hpp"
#include "../implementation/weighted_graph_separated.hpp"
#include "../implementation/bfs.hpp"
#include "../implementation/dijkstra.hpp"


template<class GraphClass>
class BFS {
private:
    using NodeHandle = typename GraphClass::NodeHandle;

    BFSHelper<GraphClass> bfs;
public:
    explicit BFS(const GraphClass &graph) : bfs(graph) {}

    std::size_t run(NodeHandle start, NodeHandle end) {
        return bfs.bfs(start, end);
    }

    [[nodiscard]] std::string_view name() const {
        return "bfs";
    }
};


template<class GraphClass>
class Dijkstra {
private:
    using NodeHandle = typename GraphClass::NodeHandle;

    DijkstraHelper<GraphClass> djikstra;
public:
    explicit Dijkstra(const GraphClass &graph) : djikstra(graph) {}

    double run(NodeHandle start, NodeHandle end) {
        return djikstra.dijkstra(start, end);
    }

    [[nodiscard]] std::string_view name() const {
        return "dijkstra";
    }
};

template<class T>
void print(std::ostream &out, const T &v, std::streamsize w) {
    out.width(w);
    out << v << " ";
    std::cout.width(w);
    std::cout << v << " ";
}


double mean(const std::vector<double> &x) {
    auto sum = std::accumulate(x.begin(), x.end(), 0.0);
    return sum / static_cast<double>(x.size());
}

double standard_deviation(const std::vector<double> &x) {
    auto m = mean(x);
    double sum = 0.0;
    for (auto x_i: x) {
        sum += (x_i - m) * (x_i - m);
    }
    return std::sqrt(sum / (static_cast<double>(x.size()) - 1.0));
}

double
duration_ms(std::chrono::high_resolution_clock::time_point t0, std::chrono::high_resolution_clock::time_point t1) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()) / 1000.;
}

void print_header_construction(std::ostream &out) {
    print(out, "\"graph class name\"", 28);
    print(out, "\"graph instance name\"", 20);
    print(out, "\"n\"", 8);
    print(out, "\"m\"", 8);
    print(out, "\"algorithm\"", 12);
    print(out, "\"graph constructor (ms)\"", 8);
    print(out, "\"algorithm constructor (ms)\"", 8);
    print(out, "\"number of queries\"", 8);
    print(out, "\"algorithm total (ms)\"", 8);
    print(out, "\"algorithm mean (ms)\"", 8);
    print(out, "\"algorithm std (ms)\"", 8);
    print(out, "\"total (ms)\"", 8);
    out << "\n";
    std::cout << "\n";
}

template<class GraphClass, class Algorithm = BFS<GraphClass>>
void
run_benchmark_construction(std::ostream &out, std::string_view graph_class_name, std::string_view graph_instance_name,
                           std::size_t num_nodes, const EdgeList &edges,
                           const std::vector<std::pair<std::size_t, std::size_t>> &queries) {
    auto t0 = std::chrono::high_resolution_clock::now();

    GraphClass graph(num_nodes, edges);

    auto t1 = std::chrono::high_resolution_clock::now();

    Algorithm algorithm(graph);

    auto t2 = std::chrono::high_resolution_clock::now();

    std::vector<double> bfs_times;
    for (const auto&[start, end]: queries) {
        auto t_bfs_start = std::chrono::high_resolution_clock::now();

        [[maybe_unused]] auto dist = algorithm.run(graph.node(start), graph.node(end));

        auto t_bfs_end = std::chrono::high_resolution_clock::now();
        bfs_times.push_back(duration_ms(t_bfs_start, t_bfs_end));
    }

    auto t3 = std::chrono::high_resolution_clock::now();

    auto construct_graph = duration_ms(t0, t1);
    auto construct_bfs = duration_ms(t1, t2);
    auto run_bfs_total = duration_ms(t2, t3);
    auto total = duration_ms(t0, t3);

    print(out, graph_class_name, 28);
    print(out, graph_instance_name, 20);
    print(out, num_nodes, 8);
    print(out, edges.size(), 8);
    print(out, algorithm.name(), 12);
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
    print(out, "\"graph class name\"", 28);
    print(out, "\"graph instance name\"", 20);
    print(out, "\"n\"", 8);
    print(out, "\"m\"", 8);
    print(out, "\"algorithm\"", 12);
    print(out, "\"distance\"", 8);
    print(out, "\"algorithm (ms)\"", 8);
    out << "\n";
    std::cout << "\n";
}

template<class GraphClass, class Algorithm = BFS<GraphClass>>
void run_benchmark_runs(std::ostream &out, std::string_view graph_class_name, std::string_view graph_instance_name,
                        std::size_t num_nodes, const EdgeList &edges,
                        const std::vector<std::pair<std::size_t, std::size_t>> &queries) {
    GraphClass graph(num_nodes, edges);
    Algorithm algorithm(graph);

    for (const auto&[start, end]: queries) {
        auto t_bfs_start = std::chrono::high_resolution_clock::now();

        auto dist = algorithm.run(graph.node(start), graph.node(end));

        auto t_bfs_end = std::chrono::high_resolution_clock::now();
        auto t = duration_ms(t_bfs_start, t_bfs_end);

        print(out, graph_class_name, 28);
        print(out, graph_instance_name, 20);
        print(out, num_nodes, 8);
        print(out, edges.size(), 8);
        print(out, algorithm.name(), 12);
        print(out, dist, 8);
        print(out, t, 8);
        out << "\n";
        std::cout << "\n";
    }
}

std::vector<std::pair<std::size_t, std::size_t>>
generate_uniform_random_queries(std::size_t num_nodes, std::size_t num_queries, std::size_t seed = 0) {
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<std::size_t> node_dist(0, num_nodes - 1);
    std::vector<std::pair<std::size_t, std::size_t>> queries;
    for (std::size_t i = 0; i < num_queries; ++i) {
        queries.emplace_back(node_dist(gen), node_dist(gen));
    }
    return queries;
}

std::vector<std::pair<std::size_t, std::size_t>>
generate_uniform_random_query(std::size_t num_nodes, std::size_t num_queries, std::size_t seed = 0) {
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<std::size_t> node_dist(0, num_nodes - 1);
    auto start = node_dist(gen);
    auto end = node_dist(gen);
    std::vector<std::pair<std::size_t, std::size_t>> queries;
    for (std::size_t i = 0; i < num_queries; ++i) {
        queries.emplace_back(start, end);
    }
    return queries;
}


void run_benchmarks_for_graphs(const std::vector<std::pair<std::string, std::string>> &graph_paths) {
    std::vector<std::tuple<std::string, std::vector<Edge>, std::size_t>> graphs;
    for (const auto &[graph_instance_path, graph_instance_name]: graph_paths) {
        const auto[edges, num_nodes] = readEdges(graph_instance_path);
        graphs.emplace_back(graph_instance_name, edges, num_nodes);
    }

    {
        std::size_t num_construction_queries = 10;
        std::size_t num_construction_repetitions = 10;

        auto file_construction = std::ofstream("benchmark-batched.csv");
        print_header_construction(file_construction);

        for (const auto &[graph_instance_name, edges, num_nodes]: graphs) {
            auto queries = generate_uniform_random_queries(num_nodes, num_construction_queries);

            for (std::size_t i = 0; i < num_construction_repetitions; ++i) {
                run_benchmark_construction<NodeGraphT<uint32_t>>(
                        file_construction, "NodeGraph<u32>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<NodeGraphT<uint64_t>>(
                        file_construction, "NodeGraph<u64>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<AdjacencyListT<uint32_t>>(
                        file_construction, "AdjacencyList<u32>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<AdjacencyListT<uint64_t>>(
                        file_construction, "AdjacencyList<u64>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<AdjacencyArrayT<uint32_t>>(
                        file_construction, "AdjacencyArray<u32>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<AdjacencyArrayT<uint64_t>>(
                        file_construction, "AdjacencyArray<u64>", graph_instance_name, num_nodes, edges, queries);

                run_benchmark_construction<WeightedGraphPairedT<uint32_t>, Dijkstra<WeightedGraphPairedT<uint32_t>>>(
                        file_construction, "WeightedGraphPaired<u32>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<WeightedGraphPairedT<uint64_t>, Dijkstra<WeightedGraphPairedT<uint64_t>>>(
                        file_construction, "WeightedGraphPaired<u64>", graph_instance_name, num_nodes, edges, queries);
                run_benchmark_construction<WeightedGraphSeparatedT<uint32_t>, Dijkstra<WeightedGraphSeparatedT<uint32_t>>>(
                        file_construction, "WeightedGraphSeparated<u32>", graph_instance_name, num_nodes, edges,
                        queries);
                run_benchmark_construction<WeightedGraphSeparatedT<uint64_t>, Dijkstra<WeightedGraphSeparatedT<uint64_t>>>(
                        file_construction, "WeightedGraphSeparated<u64>", graph_instance_name, num_nodes, edges,
                        queries);
            }
        }
    }

    {
        std::size_t num_queries = 40;

        auto file_runs = std::ofstream("benchmark-single.csv");
        print_header_runs(file_runs);

        for (const auto &[graph_instance_name, edges, num_nodes]: graphs) {
            auto queries = generate_uniform_random_queries(num_nodes, num_queries);

            run_benchmark_runs<NodeGraphT<uint32_t>>(
                    file_runs, "NodeGraph<u32>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<NodeGraphT<uint64_t>>(
                    file_runs, "NodeGraph<u64>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<AdjacencyListT<uint32_t>>(
                    file_runs, "AdjacencyList<u32>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<AdjacencyListT<uint64_t>>(
                    file_runs, "AdjacencyList<u64>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<AdjacencyArrayT<uint32_t>>(
                    file_runs, "AdjacencyArray<u32>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<AdjacencyArrayT<uint64_t>>(
                    file_runs, "AdjacencyArray<u64>", graph_instance_name, num_nodes, edges, queries);

            run_benchmark_runs<WeightedGraphPairedT<uint32_t>, Dijkstra<WeightedGraphPairedT<uint32_t>>>(
                    file_runs, "WeightedGraphPaired<u32>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<WeightedGraphPairedT<uint64_t>, Dijkstra<WeightedGraphPairedT<uint64_t>>>(
                    file_runs, "WeightedGraphPaired<u64>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<WeightedGraphSeparatedT<uint32_t>, Dijkstra<WeightedGraphSeparatedT<uint32_t>>>(
                    file_runs, "WeightedGraphSeparated<u32>", graph_instance_name, num_nodes, edges, queries);
            run_benchmark_runs<WeightedGraphSeparatedT<uint64_t>, Dijkstra<WeightedGraphSeparatedT<uint64_t>>>(
                    file_runs, "WeightedGraphSeparated<u64>", graph_instance_name, num_nodes, edges, queries);
        }
    }
}

int main() {
    const std::vector<std::pair<std::string, std::string>> graphs = {
            {"../data/netherlands.graph",   "netherlands"},
            {"../data/rgg_n_2_15_s0.graph", "rgg_n_2_15_s0"},
            {"../data/rgg_n_2_18_s0.graph", "rgg_n_2_18_s0"}
    };

    run_benchmarks_for_graphs(graphs);

    return 0;
}