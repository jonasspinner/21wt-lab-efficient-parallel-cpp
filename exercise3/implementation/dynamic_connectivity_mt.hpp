#pragma once

#include <atomic>
#include <mutex>
#include <cassert>

#include "edge_list.hpp"

#if defined(DC_B) || defined(DC_C) || defined(DC_D) || defined(DC_E)
#define USE_MUTEX
#endif

#if defined(DC_C) || defined(DC_D) || defined(DC_E) || defined(DC_F)
#define USE_COMPRESSION
#endif

#if defined(DC_D) || defined(DC_E) || defined(DC_F)
#define USE_RANKS
#endif

class DynamicConnectivity {
public:
    using Node = long;

    DynamicConnectivity() = default;

    explicit DynamicConnectivity(long num_nodes) : n(num_nodes)
#ifdef USE_MUTEX
            , union_find_mutexes(num_nodes)
#endif
    {
        union_find_parents = allocate_at_least<Node>(n);
#ifdef USE_RANKS
        union_find_ranks = allocate_at_least<Rank>(n);
#endif

        filtered_edges = allocate_at_least<std::pair<Node, Node>>(n);

        adj_index = allocate_at_least<AdjIndex>(n + 1);
        adj_counter = allocate_at_least<std::atomic<AdjIndex>>(n + 1);
        adj_edges = allocate_at_least<Node>(2 * n);

        bfs_parents = allocate_at_least<Node>(n);


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
    void addEdges(const EdgeList &edges);

    [[nodiscard]] bool connected(Node a, Node b) const {
        return find_representative(a) == find_representative(b);
    }

    // Must return -1 for roots.
    [[nodiscard]] Node parentOf(Node node) const {
        return bfs_parents[node];
    }

private:
    using Rank = unsigned int;
    using AdjIndex = unsigned long;

    /**
     * Members
     */
    Node n{0};

    // union find
    Node *union_find_parents{nullptr};
#ifdef USE_RANKS
    Rank *union_find_ranks{nullptr};
#endif
#ifdef USE_MUTEX
    std::vector<std::mutex> union_find_mutexes;
#endif

    // preliminary edge list
    std::pair<Node, Node> *filtered_edges{nullptr};
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
#endif

    template<class T>
    static T *allocate_at_least(std::size_t size) {
        return static_cast<T *>(std::aligned_alloc(alignof(T), size * sizeof(T)));
    }
};

#undef USE_MUTEX
#undef USE_COMPRESSION
#undef USE_RANKS
