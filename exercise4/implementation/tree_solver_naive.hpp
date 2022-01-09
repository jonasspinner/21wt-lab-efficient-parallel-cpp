#pragma once

#include <thread>
#include "tree.hpp"
#include "mutex_std_queue.hpp"
#include "concurrent_queue_cas_element.hpp"


// This template is beneficial for our consistent interface
template <class Tree>
class TreeSolver
{
private:
    using NodeId       = typename Tree::NodeId;
    using NodeIterator = typename Tree::NodeIterator;

public:
    explicit TreeSolver(Tree& tree) : _tree(tree){    }
    TreeSolver(const TreeSolver& src) = delete;
    TreeSolver(TreeSolver&& src)  noexcept = default;
    TreeSolver& operator=(const TreeSolver& src) = delete;
    TreeSolver& operator=(TreeSolver&& src) noexcept     {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~TreeSolver();
        new (this) TreeSolver(std::move(src));
        return *this;
    }


    void solve() {
        size_t n = m_tree.size();

        MutexStdQueue<NodeId> task_queue(n);
        task_queue.push(0);

        size_t num_threads = std::thread::hardware_concurrency();

        std::atomic<int> num_work_left = n;
        constexpr auto end_work = std::numeric_limits<NodeId>::max();

        auto do_work = [&]() {
            while (true) {
                if (num_work_left.fetch_sub(1) <= 0) return;
                NodeId current = task_queue.pop();

                auto [start, end] = m_tree.work(current);
                for (auto i = start; i < end; ++i) {
                    task_queue.push(*i);
                }

            }
        };
        std::vector<std::thread> threads(num_threads);
        for (auto& t : threads)
            t = std::thread(do_work);

        for (auto& t : threads)
            t.join();
    }

    void reset() {
        _tree.reset();
    }

private:
    Tree& m_tree;
};
