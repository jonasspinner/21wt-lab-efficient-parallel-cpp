#ifndef BFS_HPP
#define BFS_HPP

#include <vector>
#include <atomic>

#include "omp.h"

#include "adj_array.hpp"


template<class Node, class AdjIndex>
void parallel_build_adj_array(Node n, const std::pair<Node, Node> *filtered_edges, std::size_t m,
                              AdjIndex *adj_index,
                              std::atomic<AdjIndex> *adj_counter,
                              Node *adj_edges) {
    #pragma omp parallel default(none) shared(n, filtered_edges, m, adj_index, adj_counter, adj_edges)
    {
        #pragma omp for
        for (Node u = 0; u < n + 1; ++u) {
            ::new(&adj_index[u]) AdjIndex(0);
            ::new(&adj_counter[u]) std::atomic<AdjIndex>(0);
        }

        #pragma omp barrier

        #pragma omp for
        for (std::size_t i = 0; i < m; ++i) {
            auto[a, b] = filtered_edges[i];
            adj_counter[a + 1].fetch_add(1);
            adj_counter[b + 1].fetch_add(1);
        }

        #pragma omp barrier

        #pragma omp single
        {
            for (std::size_t i = 1; i < static_cast<std::size_t>(n) + 1; i++) {
                adj_counter[i] += adj_counter[i - 1];
            }
        }
        // See https://stackoverflow.com/questions/35821844/parallelization-of-a-prefix-sum-openmp
        // for parallel prefix sum

        #pragma omp barrier

        #pragma omp for
        for (std::size_t i = 0; i < static_cast<std::size_t>(n) + 1; ++i) {
            // adj_index[i] will not be modified after this store
            adj_index[i] = adj_counter[i].load();
        }

        assert(adj_counter[0] == 0);
        assert(adj_counter[n] == (AdjIndex) (2 * m));

        #pragma omp barrier

        #pragma omp for
        for (std::size_t i = 0; i < m; ++i) {
            auto[a, b] = filtered_edges[i];

            // adj_counter[a] and adj_counter[b] might be modified later
            AdjIndex a_index = adj_counter[a].fetch_add(1);
            AdjIndex b_index = adj_counter[b].fetch_add(1);

            assert(a_index < static_cast<AdjIndex>(2 * n));
            assert(b_index < static_cast<AdjIndex>(2 * n));

            // adj_index[a_index] and adj_index[b_index] will not be modified after this store
            adj_edges[a_index] = b;
            adj_edges[b_index] = a;
        }
    }
}


template<class Node, class AdjIndex>
void sequential_build_adj_array(Node n, const std::pair<Node, Node> *filtered_edges, std::size_t m,
                              AdjIndex *adj_index,
                              std::atomic<AdjIndex> *adj_counter,
                              Node *adj_edges) {

    std::uninitialized_fill_n(adj_index, n + 1, 0);
    for (Node u = 0; u < n + 1; ++u) {
        ::new(&adj_counter[u]) std::atomic<AdjIndex>(0);
    }

    for (std::size_t i = 0; i < m; ++i) {
        auto[a, b] = filtered_edges[i];
        adj_counter[a + 1].fetch_add(1);
        adj_counter[b + 1].fetch_add(1);
    }


    for (std::size_t i = 1; i < static_cast<std::size_t>(n) + 1; i++) {
        adj_counter[i] += adj_counter[i - 1];
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(n) + 1; ++i) {
        adj_index[i] = adj_counter[i].load();
    }

    assert(adj_counter[0] == 0);
    assert(adj_counter[n] == (AdjIndex) (2 * m));

    for (std::size_t i = 0; i < m; ++i) {
        auto[a, b] = filtered_edges[i];

        AdjIndex a_index = adj_counter[a].fetch_add(1);
        AdjIndex b_index = adj_counter[b].fetch_add(1);

        assert(a_index < static_cast<AdjIndex>(2 * n));
        assert(b_index < static_cast<AdjIndex>(2 * n));

        adj_edges[a_index] = b;
        adj_edges[b_index] = a;
    }
}


template<class Node, class AdjIndex>
void parallel_bfs_from_roots(Node n, const std::atomic<Node> *const union_find_parents,
                             const AdjIndex *const adj_index,
                             const Node *const adj_edges,
                             std::vector<std::vector<Node>> &bfs_frontiers,
                             std::vector<std::vector<Node>> &bfs_next_frontiers,
                             std::vector<std::vector<bool>> &bfs_visited,
                             std::atomic<Node> *bfs_parents) {

    #pragma omp parallel for default(none) shared(n, bfs_parents)
    for (Node u = 0; u < n; ++u) {
        ::new(&bfs_parents[u]) std::atomic<Node>(-1);
    }

    bfs_frontiers.resize(omp_get_max_threads());
    bfs_next_frontiers.resize(omp_get_max_threads());
    bfs_visited.resize(omp_get_max_threads());

    #pragma omp parallel default(none) shared(n, union_find_parents, adj_index, adj_edges, bfs_frontiers, bfs_next_frontiers, bfs_visited, bfs_parents)
    {
        int id = omp_get_thread_num();
        std::vector<Node> frontier, next_frontier;
        std::vector<bool> visited;

        // reuse of previously allocated memory
        // swapped with local variable to prevent false sharing between threads
        std::swap(frontier, bfs_frontiers[id]);
        std::swap(next_frontier, bfs_next_frontiers[id]);
        std::swap(visited, bfs_visited[id]);

        assert(frontier.empty());
        assert(next_frontier.empty());
        visited.assign(n, false);

        #pragma omp for schedule(dynamic)
        for (Node u = 0; u < n; ++u) {
            assert(frontier.empty());
            assert(next_frontier.empty());

            if (union_find_parents[u] == u) {
                frontier.push_back(u);
                visited[u] = true;

                std::size_t distance = 1;

                while (!frontier.empty()) {
                    while (!frontier.empty()) {
                        Node node = frontier.back();
                        frontier.pop_back();

                        for (AdjIndex i = adj_index[node]; i < adj_index[node + 1]; ++i) {
                            auto neighbor = adj_edges[i];
                            if (!visited[neighbor]) {
                                visited[neighbor] = true;
                                bfs_parents[neighbor].store(node);
                                next_frontier.push_back(neighbor);
                            }
                        }
                    }
                    distance++;
                    std::swap(frontier, next_frontier);
                }

                assert(frontier.empty());
                assert(next_frontier.empty());
            }
        }

        std::swap(frontier, bfs_frontiers[id]);
        std::swap(next_frontier, bfs_next_frontiers[id]);
        std::swap(visited, bfs_visited[id]);
    }
}

template<class Node, class AdjIndex>
void sequential_bfs_from_roots(Node n,
                               const std::atomic<Node> *const union_find_parents,
                               const AdjIndex *const adj_index,
                               const Node *const adj_edges,
                               std::vector<std::vector<Node>> &bfs_frontiers,
                               std::vector<std::vector<Node>> &bfs_next_frontiers,
                               std::vector<std::vector<bool>> &bfs_visited,
                               std::atomic<Node> *bfs_parents) {
    bfs_frontiers.resize(1);
    bfs_next_frontiers.resize(1);
    bfs_visited.resize(1);

    auto &bfs_frontier = bfs_frontiers.back();
    auto &bfs_next_frontier = bfs_next_frontiers.back();
    auto &visited = bfs_visited.back();

    bfs_frontier.reserve(n);
    bfs_next_frontier.reserve(n);
    visited.assign(n, false);

    for (Node u = 0; u < n; ++u) {
        assert(bfs_frontier.empty());
        assert(bfs_next_frontier.empty());
        if (union_find_parents[u] != u) {
            continue;
        }

        bfs_parents[u] = -1;

        bfs_frontier.push_back(u);
        visited[u] = true;

        while (!bfs_frontier.empty()) {
            while (!bfs_frontier.empty()) {
                Node node = bfs_frontier.back();
                bfs_frontier.pop_back();

                for (AdjIndex i = adj_index[node]; i < adj_index[node + 1]; ++i) {
                    auto neighbor = adj_edges[i];
                    if (!visited[neighbor]) {
                        visited[neighbor] = true;
                        bfs_parents[neighbor] = node;
                        bfs_next_frontier.push_back(neighbor);
                    }
                }
            }
            std::swap(bfs_frontier, bfs_next_frontier);
        }

        assert(bfs_frontier.empty());
        assert(bfs_next_frontier.empty());
    }
}

#endif //BFS_HPP
