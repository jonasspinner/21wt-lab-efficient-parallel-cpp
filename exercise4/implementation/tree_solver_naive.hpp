#pragma once

#include <thread>
#include "tree.hpp"
#include "mutex_std_queue.hpp"
#include "concurrent_queue_cas_element.hpp"


// This template is beneficial for our consistent interface
template<class Tree>
class TreeSolverNaive {
private:
    using NodeId = typename Tree::NodeId;
    using NodeIterator = typename Tree::NodeIterator;

public:
    explicit TreeSolverNaive(Tree &tree) : TreeSolverNaive(tree, tree.size(), std::thread::hardware_concurrency()) {}

    explicit TreeSolverNaive(Tree &tree, size_t capacity, size_t num_threads) : m_tree(tree), m_capacity(capacity),
                                                                                m_task_queue(m_capacity),
                                                                                m_num_threads(num_threads) {}

    TreeSolverNaive(const TreeSolverNaive &src) = delete;

    TreeSolverNaive(TreeSolverNaive &&src) noexcept = default;

    TreeSolverNaive &operator=(const TreeSolverNaive &src) = delete;

    TreeSolverNaive &operator=(TreeSolverNaive &&src) noexcept {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~TreeSolver();
        new(this) TreeSolverNaive(std::move(src));
        return *this;
    }


    void solve() {
        size_t n = m_tree.size();

        m_task_queue.push(1);

        std::atomic<int> num_work_left = n;

        auto do_work = [&]() {
            while (true) {
                if (num_work_left.fetch_sub(1) <= 0) return;
                NodeId current = m_task_queue.pop() - 1;

                auto[start, end] = m_tree.work(current);
                for (auto i = start; i < end; ++i) {
                    m_task_queue.push(*i + 1);
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
        m_tree.reset();
        m_task_queue.reset();
    }

private:
    Tree &m_tree;

    size_t m_capacity;
    MutexStdQueue<NodeId> m_task_queue;
    size_t m_num_threads;
};
