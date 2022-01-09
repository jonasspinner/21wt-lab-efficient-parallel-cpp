#pragma once

#include <thread>
#include <iomanip>
#include "tree.hpp"
#include "mutex_std_queue.hpp"
#include "concurrent_container.hpp"
#include "concurrent_queue.hpp"
#include "utils/misc.h"


// This template is beneficial for our consistent interface
template<class Tree, bool keep_stats = false>
class TreeSolver {
private:
    using NodeId = typename Tree::NodeId;
    using NodeIterator = typename Tree::NodeIterator;

    constexpr static NodeId done_task = std::numeric_limits<NodeId>::max();

    template<class T>
    struct LocalQueue {
        alignas(64) std::queue<T> m_queue{};

        T pop() {
            T e = m_queue.front();
            m_queue.pop();
            return e;
        }

        void push(T e) {
            m_queue.push(e);
        }

        auto size() const { return m_queue.size(); }

        auto empty() const { return m_queue.empty(); }
    };

    struct Stats {
        std::mutex m_mutex;
        std::vector<std::chrono::high_resolution_clock::time_point> m_time_points;
        std::vector<size_t> m_global_queue_size;
        std::vector<std::vector<size_t>> m_local_queue_sizes;
    };

public:
    explicit TreeSolver(Tree &tree) : TreeSolver(tree, tree.size(), std::thread::hardware_concurrency()) {
        // Here you could do some precomputation
    }

    explicit TreeSolver(Tree &tree, size_t task_queue_capacity, size_t num_threads)
            : m_tree(tree), m_global_queue_capacity(task_queue_capacity), m_global_queue(m_global_queue_capacity),
              m_num_threads(num_threads == 0 ? std::thread::hardware_concurrency() : num_threads),
              m_local_queues(m_num_threads) {
        // Here you could do some precomputation
        if constexpr(keep_stats) {
            m_stats.m_local_queue_sizes.resize(m_num_threads);
        }
    }

    TreeSolver(const TreeSolver &src) = delete;

    TreeSolver(TreeSolver &&src) noexcept = default;

    TreeSolver &operator=(const TreeSolver &src) = delete;

    TreeSolver &operator=(TreeSolver &&src)
    noexcept {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~TreeSolver();
        new(this) TreeSolver(std::move(src));
        return *this;
    }


    void solve() {
        auto n = m_tree.size();

        NodeId start_node = 0;
        m_global_queue.push(start_node + 1);

        std::atomic<int> num_work_left = n;
        std::atomic<int> num_elements_in_queues = 0;

        auto do_work = [&](size_t thread_id) {
            LocalQueue<NodeId> &local_queue = m_local_queues[thread_id];
            while (true) {
                if constexpr(keep_stats) {
                    lock_stats_and_local_queues();

                    m_stats.m_time_points.push_back(std::chrono::high_resolution_clock::now());
                    m_stats.m_global_queue_size.push_back(m_global_queue.size());
                    for (size_t i = 0; i < m_num_threads; ++i) {
                        m_stats.m_local_queue_sizes[i].push_back(m_local_queues[i].size());
                    }

                    unlock_stats_and_local_queues();
                }

                NodeId current;
                if (!local_queue.empty()) {
                    lock_stats_and_local_queues();

                    current = local_queue.pop();
                    num_work_left.fetch_sub(1);

                    unlock_stats_and_local_queues();
                } else {
                    if (num_work_left.fetch_sub(1) <= 0) {
                        for (size_t i = 0; i < m_num_threads; ++i) {
                            m_global_queue.push(done_task);
                        }
                        return;
                    };
                    current = m_global_queue.pop();
                }
                num_elements_in_queues.fetch_sub(1, std::memory_order_relaxed);

                if (current == done_task) return;
                current--;

                NodeId *start;
                NodeId *end;
                std::tie(start, end) = m_tree.work(current);
                auto num_new_elements = end - start;

                auto total_elements =
                        std::max(num_elements_in_queues.load(std::memory_order_relaxed), 0) + num_new_elements;
                auto avg_elements_per_queue = total_elements / m_num_threads;

                size_t desired_elements_in_local_queue = std::max<int>(1, avg_elements_per_queue);
                size_t max_desired_elements_in_local_queue = desired_elements_in_local_queue * 1.5;

                /*
                std::cout
                << thread_id << " local size " << local_queue.size()
                << " new elements " << num_new_elements
                << " total elements " << total_elements
                << " global queue " << m_global_queue.size()
                << " desired elements " << desired_elements_in_local_queue
                << " max desired elements " << max_desired_elements_in_local_queue
                << std::endl;
                 */

                num_elements_in_queues.fetch_add(num_new_elements, std::memory_order_relaxed);

                lock_stats_and_local_queues();

                auto child_it = start;
                while (child_it < end && local_queue.size() < desired_elements_in_local_queue) {
                    local_queue.push(*child_it + 1);
                    ++child_it;
                }

                while (child_it < end) {
                    m_global_queue.push(*child_it + 1);
                    ++child_it;
                }

                while (local_queue.size() > max_desired_elements_in_local_queue) {
                    auto e = local_queue.pop();
                    m_global_queue.push(e);
                }

                unlock_stats_and_local_queues();
            }
        };
        std::vector<std::thread> threads;
        threads.reserve(m_num_threads);
        for (size_t thread_id = 0; thread_id < m_num_threads; ++thread_id) {
            threads.emplace_back(do_work, thread_id);
        }

        for (auto &t: threads)
            t.join();
    }

    void reset() {
        m_tree.reset();
        // NOTE: reset your own data structures
        m_global_queue.reset();
        if constexpr(keep_stats) {
            m_stats.m_time_points.resize(0);
            m_stats.m_global_queue_size.resize(0);
            for (int thread_id = 0; thread_id < m_num_threads; ++thread_id) {
                m_stats.m_local_queue_sizes[thread_id].resize(0);
            }
        }
    }

    const Stats & stats() {
        if constexpr(keep_stats) {
            return m_stats;
        } else {
            throw std::logic_error("");
        }
    }

private:
    Tree &m_tree;
    // NOTE: some additional data structures
    size_t m_global_queue_capacity{};
    MutexStdQueue<NodeId> m_global_queue{};
    size_t m_num_threads{};
    std::vector<LocalQueue<NodeId>> m_local_queues;

    // NOTE: Only used if keep_stats is true.
    Stats m_stats;

    void lock_stats_and_local_queues() {
        if constexpr(keep_stats) {
            m_stats.m_mutex.lock();
        }
    };

    void unlock_stats_and_local_queues() {
        if constexpr(keep_stats) {
            m_stats.m_mutex.unlock();
        }
    };
};
