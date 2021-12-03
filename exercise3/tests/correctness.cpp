// some tests written by Lucas Alber were incorporated to this test
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <set>
#include <map>

#include "implementation/dynamic_connectivity.hpp"
#include "implementation/num_components.hpp"
#include "implementation/edge_list.hpp"
#include "utils/commandline.hpp"


template<typename Implementation>
void test_components(const std::string& name, const EdgeList& edges,
                     std::size_t num_blocks, std::size_t num_nodes,
                     const EdgeList& to_check)
{
    std::cout << "Testing components     on " << name << "  ..." << std::flush;

    const auto block_size = edges.size() / num_blocks;
    Implementation dc(num_nodes);

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

    std::cout << " ok" << std::endl;
}

template<typename Implementation>
void test_parents(std::string name, EdgeList& edges, std::size_t num_nodes)
{
    std::cout << "Testing parents        on " << name << "  ..." << std::flush;

    Implementation dc(num_nodes);
    dc.addEdges(edges);


    std::vector<std::set<std::size_t>> graph(num_nodes, std::set<std::size_t>());
    for (auto e : edges) {
        graph[e.from].insert(e.to);
        graph[e.to].insert(e.from);
    }

    for (long node = 0; node < long(num_nodes); node++) {
        long parent = dc.parentOf(node);
        if (parent == -1) continue;

        if (graph[node].find(parent) == graph[node].end()) {
            throw std::runtime_error{std::to_string(parent)
                    + " cannot be parent of " + std::to_string(node)
                    + " because the two are not connected!"};
        }
    }

    std::cout << " ok" << std::endl;
}

template<typename Implementation>
void test_num_components(std::string name, EdgeList& edges, std::size_t num_nodes)
{
    std::cout << "Testing num components on " << name << "  ..." << std::flush;

    ComponentsCounter comp(num_nodes);

    std::size_t seq_components = comp.addEdges(edges);

    Implementation dc(num_nodes);
    dc.addEdges(edges);

    std::size_t components = 0;
    for (std::size_t i = 0; i < num_nodes; i++) {
        if (dc.parentOf(i) == -1) components++;
    }


    if (seq_components != components) {
        throw std::runtime_error{std::to_string(seq_components)
                + " components expected but got " + std::to_string(components)};
    }

    std::cout << " ok" << std::endl;
}

// author: Lucas Alber
int main(int argn, char** argc)
{
    CommandLine c(argn, argc);

    std::vector<std::pair<std::string, std::string>> tests;
    tests.reserve(3);
    tests.push_back(std::make_pair("../data/test_graph1.graph","../data/test_correctness1.graph"));
    tests.push_back(std::make_pair("../data/test_graph2.graph","../data/test_correctness2.graph"));

    std::string graph      = c.strArg("-graph");
    std::string test_graph = c.strArg("-test");
    if (!graph.empty()) {
        tests.clear();
        tests.push_back(std::make_pair(graph, test_graph));
    }

    for (auto test : tests) {
        EdgeList edges;
        EdgeList to_check;
        std::size_t num_nodes;
        std::size_t num_blocks;
        std::tie(edges, num_nodes)     = readEdges(test.first);
        std::tie(to_check, num_blocks) = readEdges(test.second);

        test_components<DynamicConnectivity>(test.first, edges, num_blocks, num_nodes, to_check);
        test_parents<DynamicConnectivity>(graph, edges, num_nodes);
        test_num_components<DynamicConnectivity>(test.first, edges, num_nodes);
    }

    return 0;
}
