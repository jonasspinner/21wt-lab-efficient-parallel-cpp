#pragma once

#include <thread>
#include "tree.hpp"
#include "mutex_std_queue.hpp"
#include "concurrent_queue_cas_element.hpp"


// This template is beneficial for our consistent interface
template<class Tree>
class TreeSolverSequential {
private:
    using NodeId = typename Tree::NodeId;
    using NodeIterator = typename Tree::NodeIterator;

public:
    explicit TreeSolverSequential(Tree &tree) : _tree(tree) {
        // Here you could do some precomputation
    }

    TreeSolverSequential(const TreeSolverSequential &src) = delete;

    TreeSolverSequential(TreeSolverSequential &&src) noexcept = default;

    TreeSolverSequential &operator=(const TreeSolverSequential &src) = delete;

    TreeSolverSequential &operator=(TreeSolverSequential &&src) noexcept {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~TreeSolver();
        new(this) TreeSolverSequential(std::move(src));
        return *this;
    }


    void solve() {
        // NOTE: This could be an example (that works sequentially)
        std::vector<NodeId> task_queue;
        task_queue.push_back(0);
        while (task_queue.size()) {
            NodeIterator start;
            NodeIterator end;
            NodeId current = task_queue.back();
            task_queue.pop_back();
            std::tie(start, end) = _tree.work(current);
            for (NodeIterator i = start; i < end; ++i) {
                task_queue.push_back(*i);
            }
        }
    }

    void reset() {
        _tree.reset();
    }

private:
    Tree &_tree;
};
