#pragma once

#include <atomic>
#include <mutex>
#include <cassert>

#include "omp.h"

#include "edge_list.hpp"
#include "utils/allocation.h"


class DynamicConnectivity {
    /**
     * Based on [2019 Alistarh et al].
     *
     *
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

#ifdef USE_MUTEX
#define TRY_LOCK_TREES_A
#ifdef TRY_LOCK_TREES_A

    bool try_lock_trees(Node &a, Node &b) {
        static int i = 0;
        constexpr bool print_event = false;
        // We keep the invariant a < b and a has been locked before b
        assert(a != b);
        if (a > b) {
            std::swap(a, b);
        }
        assert(a < b);
        union_find_mutexes[a].lock();
        union_find_mutexes[b].lock();

        while (true) {
            Node a_parent = union_find_parents[a].load();
            if (a_parent != a) {
                assert(a < b);
                // a is no longer a root
                if (a_parent == b) {
                    if constexpr (print_event)
                        std::cerr << i++ << " ### a_parent == b  -  a and b already in the same tree"
                                << " a=" << a << " b=" << b << std::endl; // happens
                    // a and b are already in the same tree
                    union_find_mutexes[b].unlock();
                    union_find_mutexes[a].unlock();
                    return false;
                }
                if (a_parent > b) {
                    if constexpr (print_event) std::cerr << i++ << " ### a_parent > b  -  update a" << " a=" << a << " b=" << b << std::endl;
                    union_find_mutexes[a_parent].lock();
                    union_find_mutexes[a].unlock();
                    a = a_parent;
                    std::swap(a, b);
                    continue;
                } else if (a_parent < b) {
                    if constexpr (print_event)
                        std::cerr << i++ << " ### a_parent < b  -  update a and relock b" << " a=" << a << " b=" << b << std::endl; // happens
                    union_find_mutexes[b].unlock();
                    union_find_mutexes[a_parent].lock();
                    union_find_mutexes[a].unlock();
                    union_find_mutexes[b].lock();
                    a = a_parent;
                }
            }

            assert(a < b);
            assert(union_find_mutexes[a].try_lock() == false);
            assert(union_find_mutexes[b].try_lock() == false);

            Node b_parent = union_find_parents[b].load();
            if (b_parent != b) {
                assert(a < b);
                // b is no longer b root
                if (b_parent == a) {
                    if constexpr (print_event)
                        std::cerr << i++ << " ### b_parent == a  -  a and b already in the same tree"
                                  << " a=" << a << " b=" << b << std::endl; // happens
                    // a and b are already in the same tree
                    union_find_mutexes[b].unlock();
                    union_find_mutexes[a].unlock();
                    return false;
                }
                if (b_parent > a) {
                    if constexpr (print_event)
                        std::cerr << i++ << " ### b_parent > a  -  update b" << " a=" << a << " b=" << b << std::endl; // happens
                    union_find_mutexes[b_parent].lock();
                    union_find_mutexes[b].unlock();
                    b = b_parent;
                } else if (b_parent < a) {
                    if constexpr (print_event)
                        std::cerr << i++ << " ### b_parent < a  -  update b and relock a" << " a=" << a << " b=" << b << std::endl;
                    union_find_mutexes[a].unlock();
                    union_find_mutexes[b_parent].lock();
                    union_find_mutexes[b].unlock();
                    union_find_mutexes[a].lock();
                    b = b_parent;
                    std::swap(a, b);
                }
            }

            assert(a < b);
            assert(union_find_mutexes[a].try_lock() == false);
            assert(union_find_mutexes[b].try_lock() == false);

            if (union_find_parents[a].load() == a && union_find_parents[b].load() == b) {
                return true;
            }
        }
    }

#endif
#ifdef TRY_LOCK_TREES_B
    bool try_lock_trees(Node &a, Node &b) {
        static int i = 0;
        constexpr bool print_event = true;
        if (a > b) std::swap(a, b);

        //std::lock(union_find_mutexes[a], union_find_mutexes[b]);
        union_find_mutexes[a].lock();
        union_find_mutexes[b].lock();

        while (true) {
            if (union_find_parents[a] != a  || union_find_parents[b] != b) {
                if constexpr(print_event) std::cerr << i++ << " ### re run" << std::endl;
                union_find_mutexes[b].unlock();
                union_find_mutexes[a].unlock();
                a = find_representative(a);
                b = find_representative(b);
                if (a == b) return false;
                if (a > b) std::swap(a, b);
                //std::lock(union_find_mutexes[a], union_find_mutexes[b]);
                union_find_mutexes[a].lock();
                union_find_mutexes[b].lock();
            } else {
                assert(a < b);
                assert(union_find_mutexes[a].try_lock() == false);
                assert(union_find_mutexes[b].try_lock() == false);

                assert(union_find_parents[a] == a);
                assert(union_find_parents[b] == b);
                assert(union_find_parents[a] != union_find_parents[b]);

                return true;
            }
        }
    }
#endif
#endif

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
