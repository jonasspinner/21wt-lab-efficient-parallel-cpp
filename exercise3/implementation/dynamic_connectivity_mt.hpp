#pragma once

#include <atomic>
#include <mutex>
#include <cassert>

#include "edge_list.hpp"
#include "graph_algorithms.hpp"
#include "utils/allocation.h"


#if defined(DC_C) || defined(DC_D) || defined(DC_E)
#define USE_COMPRESSION
#endif

#if defined(DC_D) || defined(DC_E)
#define USE_RANKS
#endif

class DynamicConnectivity {
public:
    using Node = int;
    using Rank = unsigned int;
    using AdjIndex = unsigned int;

    DynamicConnectivity() = default;

    explicit DynamicConnectivity(long num_nodes) : n(num_nodes), union_find_mutexes(num_nodes) {
        if (num_nodes >= std::numeric_limits<Node>::max())
            throw std::runtime_error("Node type to small");

        union_find_parents = allocation::allocate_at_least<Node>(n);
#ifdef USE_RANKS
        union_find_ranks = allocation::allocate_at_least<Rank>(n);
#endif

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
                ::new(&union_find_parents[u]) std::atomic<Node>(u);
            }
#ifdef USE_RANKS
#pragma omp for
            for (Node u = 0; u < n; ++u) {
                ::new(&union_find_ranks[u]) std::atomic<Rank>(0);
            }
#endif

            // Will be written again in bfs, but this way parentOf() is valid without a call to addEdges()
#pragma omp for
            for (Node u = 0; u < n; ++u) {
                ::new(&bfs_parents[u]) Node(-1);
            }
        }

#ifndef NDEBUG
        for (Node u = 0; u < n; ++u) {
            assert(union_find_parents[u] == u);
            assert(bfs_parents[u] == -1);
        }
#endif
    }

    ~DynamicConnectivity() {
        free(union_find_parents);
#ifdef USE_RANKS
        free(union_find_ranks);
#endif
        free(filtered_edges);
        num_filtered_edges.store(0);

        free(adj_index);
        free(adj_counter);
        free(adj_edges);

        free(bfs_parents);
    }

    // should contain #pragma omp parallel for
    void addEdges(const EdgeList &edges) {
        std::size_t num_edges = edges.size();
        #pragma omp parallel default(none), shared(edges, num_edges, std::cerr)
        {
            // If omp_get_max_threads() was smaller during construction than omp_get_num_threads() is now
            #pragma omp single
            filtered_edges_per_thread.resize(omp_get_num_threads());

            int id = omp_get_thread_num();

            // Localize vector to prevent false sharing
            std::vector<std::pair<Node, Node>> filtered_edges_local;
            std::swap(filtered_edges_per_thread[id], filtered_edges_local);

            #pragma omp for
            for (std::size_t i = 0; i < num_edges; ++i) {
                Node a = static_cast<Node>(edges[i].from);
                Node b = static_cast<Node>(edges[i].to);

                assert(a < n && b < n);

                if (unite(a, b)) {
                    filtered_edges_local.emplace_back(a, b);
                }
            }

            // Only increase num_filtered_edges once per thread
            auto pos = num_filtered_edges.fetch_add(filtered_edges_local.size());
            for (auto[a, b]: filtered_edges_local) {
                ::new(&filtered_edges[pos]) std::pair<Node, Node>(a, b);
                pos++;
            }

            // Give back allocated memory
            filtered_edges_local.clear();
            std::swap(filtered_edges_per_thread[id], filtered_edges_local);
        }

        auto is_root = [&](Node u) { return union_find_parents[u] == u; };

#ifdef DC_E
        parallel_build_adj_array(n, filtered_edges, num_filtered_edges.load(), adj_index, adj_counter, adj_edges);
        parallel_bfs_from_roots(n, is_root, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers, bfs_visited,
                            bfs_parents);
#else
        sequential_build_adj_array(n, filtered_edges, num_filtered_edges.load(), adj_index, adj_counter, adj_edges);
        sequential_bfs_from_roots(n, is_root, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers,
                                  bfs_visited, bfs_parents);
#endif
    }

    [[nodiscard]] bool connected(Node a, Node b) const {
        return find_representative(a) == find_representative(b);
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
    Node *union_find_parents{nullptr};
#ifdef USE_RANKS
    Rank *union_find_ranks{nullptr};
#endif
    std::vector<std::mutex> union_find_mutexes;

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

#ifdef USE_COMPRESSION

    [[nodiscard]] Node find_representative_and_compress(Node node);

#endif

    bool unite(Node a, Node b);


#define TRY_LOCK_TREES_C
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
            Node a_parent = union_find_parents[a];
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

            Node b_parent = union_find_parents[b];
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

            if (union_find_parents[a] == a && union_find_parents[b] == b) {
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
#ifdef TRY_LOCK_TREES_C

    bool try_lock_trees(Node &a, Node &b) {
        while (true) {
#ifdef USE_COMPRESSION
            a = find_representative_and_compress(a);
            b = find_representative_and_compress(b);
#else
            a = find_representative(a);
            b = find_representative(b);
#endif

            if (a == b) return false;

            if (a > b) std::swap(a, b);
            if (!union_find_mutexes[a].try_lock()) continue;
            if (union_find_parents[a] != a || !union_find_mutexes[b].try_lock()) {
                union_find_mutexes[a].unlock();
                continue;
            }
            if (union_find_parents[b] != b) {
                union_find_mutexes[b].unlock();
                union_find_mutexes[a].unlock();
                continue;
            }
            return true;
        }
    }

#endif
};

#undef USE_COMPRESSION
#undef USE_RANKS
