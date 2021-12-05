#pragma once

#undef DC_F

#if defined(DC_SEQUENTIAL) + defined(DC_A) + defined(DC_B) + defined(DC_C) + defined(DC_D) + defined(DC_E) + defined(DC_F) > 1
#error Please choose at most one implementation
#endif

#if defined(DC_SEQUENTIAL) || defined(DC_A)
#include "implementation/dynamic_connectivity_sequential.hpp"
#elif defined(DC_B) || defined(DC_C) || defined(DC_D) || defined(DC_E)
#include "implementation/dynamic_connectivity_mt.hpp"
#else
#if !defined(DC_F)
#define DC_F
#warning Default choice: DC_F \
         dynamic_connectivity.cpp is the corresponing .cpp file \
         Specify DC_A / DC_SEQUENTIAL, DC_B, DC_C, DC_D, DC_E of DC_F explicitly to silency warning
#endif

#include <atomic>
#include <mutex>
#include <cassert>

#include "omp.h"

#include "edge_list.hpp"
#include "utils/allocation.h"


class DynamicConnectivity {
    /**
     * Based on [2019 Alistarh et al]. Union find parent and rank structure is stored in `union_find`. To differentiate
     * parent and and rank data, the sign bit is used. All non-negative values are node data and negative values are
     * rank data.
     * */
/*
@InProceedings{alistarh_et_al:LIPIcs:2020:11801,
  author =	{Dan Alistarh and Alexander Fedorov and Nikita Koval},
  title =	{{In Search of the Fastest Concurrent Union-Find Algorithm}},
  booktitle =	{23rd International Conference on Principles of Distributed Systems (OPODIS 2019)},
  pages =	{15:1--15:16},
  series =	{Leibniz International Proceedings in Informatics (LIPIcs)},
  ISBN =	{978-3-95977-133-7},
  ISSN =	{1868-8969},
  year =	{2020},
  volume =	{153},
  editor =	{Pascal Felber and Roy Friedman and Seth Gilbert and Avery Miller},
  publisher =	{Schloss Dagstuhl--Leibniz-Zentrum fuer Informatik},
  address =	{Dagstuhl, Germany},
  URL =		{https://drops.dagstuhl.de/opus/volltexte/2020/11801},
  URN =		{urn:nbn:de:0030-drops-118012},
  doi =		{10.4230/LIPIcs.OPODIS.2019.15},
  annote =	{Keywords: union-find, concurrency, evaluation, benchmarks, hardware transactional memory}
}
*/
public:
    using Node = int;
    using AdjIndex = unsigned int;

    static_assert(std::is_signed_v<Node>, "Node must be signed to allow storing of ranks for roots.");
    DynamicConnectivity() = default;

    explicit DynamicConnectivity(long num_nodes) : n(num_nodes)
    {
        if (num_nodes >= std::numeric_limits<Node>::max())
            throw std::runtime_error("Node type to small. Change Node and AdjIndex to be larger types.");

        union_find = allocation::allocate_at_least<std::atomic<Node>>(n);

        filtered_edges = allocation::allocate_at_least<std::pair<Node, Node>>(n);
        filtered_edges_per_thread.resize(omp_get_max_threads());
        for (auto &f : filtered_edges_per_thread) {
            f.reserve(n / std::max<std::size_t>(filtered_edges_per_thread.size() - 1, 1));
        }

        adj_index = allocation::allocate_at_least<AdjIndex>(n + 1);
        adj_counter = allocation::allocate_at_least<std::atomic<AdjIndex>>(n + 1);
        adj_edges = allocation::allocate_at_least<Node>(2 * n);

        bfs_parents = allocation::allocate_at_least<Node>(n);


#pragma omp parallel default(none)
        {

#pragma omp for
            for (Node u = 0; u < n; ++u) {
                ::new(&union_find[u]) std::atomic<Node>(to_rank_repr(1));
            }

#pragma omp for
            for (Node u = 0; u < n; ++u) {
                ::new(&bfs_parents[u]) Node(-1);
            }
        }

#ifndef NDEBUG
        for (Node u = 0; u < n; ++u) {
            assert(union_find[u] == to_rank_repr(1));
            assert(bfs_parents[u] == -1);
        }
#endif
    }

    ~DynamicConnectivity() {
        free(union_find);

        free(filtered_edges);
        num_filtered_edges.store(0);

        free(adj_index);
        free(adj_counter);
        free(adj_edges);

        free(bfs_parents);
    }

    // should contain #pragma omp parallel for
    void addEdges(const EdgeList &edges);

    [[nodiscard]] bool connected(Node a, Node b) const {
        while (true) {
            a = find_representative(a);
            b = find_representative(b);
            assert(is_node_repr(a) && is_node_repr(b));
            if (a == b) return true;
            if (is_rank_repr(union_find[a]))
                return false;
        }
    }

    // Must return -1 for roots.
    [[nodiscard]] Node parentOf(Node node) const {
        return bfs_parents[node];
    }

private:
    /**
     * Members
     */
    Node n{0};

    // union find
    std::atomic<Node> *union_find{nullptr};

    // preliminary edge list
    std::pair<Node, Node> *filtered_edges{nullptr};
    std::vector<std::vector<std::pair<Node, Node>>> filtered_edges_per_thread;
    std::atomic<std::size_t> num_filtered_edges{0};

    // preliminary graph
    AdjIndex *adj_index{nullptr};
    std::atomic<AdjIndex> *adj_counter{nullptr};
    Node *adj_edges{nullptr};

    // bfs
    std::vector<std::vector<Node>> bfs_frontiers;
    std::vector<std::vector<Node>> bfs_next_frontiers;
    std::vector<std::vector<bool>> bfs_visited;
    Node *bfs_parents{nullptr};


    [[nodiscard]] Node find_representative(Node node) const;

    [[nodiscard]] std::pair<Node, Node> find_representative_and_compress(Node node);


    bool unite(Node a, Node b);

    constexpr static bool is_rank_repr(Node repr) {
        return repr < 0;
    }

    constexpr static bool is_node_repr(Node repr) {
        return repr >= 0;
    }

    constexpr static Node to_rank_repr(Node rank) {
        return -rank;
    }

    constexpr static Node from_rank_repr(Node repr) {
        return -repr;
    }
};

#endif
