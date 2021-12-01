#pragma once

#include "tree.hpp"


// This template is beneficial for our consistent interface
template <class Tree>
class TreeSolver
{
private:
    using NodeId       = typename Tree::NodeId;
    using NodeIterator = typename Tree::NodeIterator;

public:
    TreeSolver(Tree& tree) : _tree(tree)
    {
        // Here you could do some precomputation
    }
    TreeSolver(const TreeSolver& src) = delete;
    TreeSolver(TreeSolver&& src) = default;
    TreeSolver& operator=(const TreeSolver& src) = delete;
    TreeSolver& operator=(TreeSolver&& src)
    {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~TreeSolver();
        new (this) TreeSolver(std::move(src));
        return *this;
    }


    void solve()
    {
        // TODO Implement this function this could be an example
        // (that works sequentially)
        std::vector<NodeId> task_queue;
        task_queue.push_back(0);
        while (task_queue.size())
        {
            NodeIterator start;
            NodeIterator end;
            NodeId current = task_queue.back();
            task_queue.pop_back();
            std::tie(start, end) = _tree.work(current);
            for (NodeIterator i = start; i<end; ++i)
            {
                task_queue.push_back(*i);
            }
        }
    }

    void reset()
    {
        _tree.reset();
        // TODO reset your own data structures
    }

private:
    Tree& _tree;
    // TODO some additional data structures
};
