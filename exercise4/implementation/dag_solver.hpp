#pragma once

#include <thread>
#include <cassert>
#include <algorithm>

#include "dag.hpp"
#include "mutex_std_queue.hpp"
#include "utils/misc.h"

// This template is beneficial for our consistent interface
template<class DAG = DAGTask>
class DAGSolver {
private:
    using NodeId = typename DAG::NodeId;
    using NodeIterator = typename DAG::NodeIterator;

    using NodeState = int;
    constexpr static NodeState initial = 0;
    constexpr static NodeState in_queue = 1;
    constexpr static NodeState in_progress = 2;
    constexpr static NodeState done = 3;

    constexpr static NodeId done_task = std::numeric_limits<NodeId>::max();
public:
    explicit DAGSolver(DAG &dag)
            : DAGSolver(dag, dag.size(), std::thread::hardware_concurrency()) {}

    explicit DAGSolver(DAG &dag, size_t task_queue_capacity, size_t num_threads)
            : m_dag(dag), m_task_queue_capacity(task_queue_capacity), m_task_queue(m_task_queue_capacity),
              m_num_threads(num_threads == 0 ? std::thread::hardware_concurrency() : num_threads),
              m_state(std::make_unique<std::atomic<NodeState>[]>(m_dag.size())) {
        // Here you could do some precomputation (i.e. explore the graph)
        for (NodeId i = 0; i < m_dag.size(); ++i) {
            m_state[i] = initial;
        }
    }

    DAGSolver(const DAGSolver &src) = delete;

    DAGSolver(DAGSolver &&src) noexcept = default;

    DAGSolver &operator=(const DAGSolver &src) = delete;

    DAGSolver &operator=(DAGSolver &&src)
    noexcept {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~DAGSolver();
        new(this) DAGSolver(std::move(src));
        return *this;
    }


    void solve() {
        size_t n = m_dag.size();

        std::atomic<int> num_work_left = (int)n;


        NodeId start_node = 0;
        m_task_queue.push(start_node + 1);
        m_state[start_node] = in_queue;


        auto do_work = [&]() {
            while (true) {
                NodeId current = m_task_queue.pop();
                if (current == done_task) return;

                current--;

                assert(state[current].load() == in_queue);

                auto[in_start, in_end] = m_dag.incoming(current);
                bool all_parents_done = std::all_of(in_start, in_end,
                                                    [&](auto parent) {
                                                        return m_state[parent].load() == done;
                                                    });

                if (!all_parents_done) {
                    m_task_queue.push(current + 1);
                    continue;
                }

                m_state[current] = in_progress;

                m_dag.work(current);

                m_state[current] = done;
                if (num_work_left.fetch_sub(1) <= 1) {
                    for (size_t i = 0; i < m_num_threads; ++i) {
                        m_task_queue.push(done_task);
                    }
                }

                for (auto child: utils::Range{m_dag.outgoing(current)}) {
                    int expected_child_state = initial;
                    if (m_state[child].compare_exchange_strong(expected_child_state, in_queue)) {
                        m_task_queue.push(child + 1);
                    }
                }
            }
        };
        std::vector<std::thread> threads(m_num_threads);
        for (auto &t: threads)
            t = std::thread(do_work);

        for (auto &t: threads)
            t.join();
    }

    void reset() {
        m_dag.reset();
        // NOTE: reset your own data structures
        m_task_queue.reset();
        for (NodeId i = 0; i < m_dag.size(); ++i) {
            m_state[i] = initial;
        }
    }

private:
    DAG &m_dag;

    // NOTE: some additional data structures
    size_t m_task_queue_capacity{};
    MutexStdQueue<NodeId> m_task_queue{};
    size_t m_num_threads{};
    std::unique_ptr<std::atomic<NodeState>[]> m_state;
};
