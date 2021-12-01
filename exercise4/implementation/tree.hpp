#pragma once

#include <cstddef>
#include <fstream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <limits>

// DO NOT CHANGE THIS

class TreeTask
{

public:
    using NodeId       = uint;
    using NodeIterator = uint*;

    TreeTask(std::string file, double work_factor = 1)
    {
        std::ifstream in(file);

        in >> n;

        nodes       = std::make_unique<size_t[]>(n+1);
        work_amount = std::make_unique<size_t[]>(n);
        edges       = std::make_unique<NodeId[]>(n);
        parent      = std::make_unique<NodeId[]>(n);
        done        = std::make_unique<std::atomic_char[]>(n+1);

        for (size_t i = 0; i<n; ++i)
        {
            done[i].store(0);
        }

        size_t out, temp, work;
        size_t cur_e = 0;
        for (size_t ni = 0; ni < n; ++ni)
        {
            in >> out;
            nodes[ni] = cur_e;
            for (; cur_e < nodes[ni] + out; ++cur_e)
            {
                in >> temp;
                edges[cur_e] = temp;
                parent[temp] = ni;
            }

            in >> work;
            work_amount[ni] = work * work_factor;
        }

        // dummies
        nodes[n]  = cur_e;
        parent[0] = n;
        done[n].store(1);
    }

    // returns the start and the end iterator for the child nodes
    std::pair<NodeIterator, NodeIterator>
    inline work(NodeId id)
    {
        if (done[id].load()) throw std::runtime_error{"Already worked on this node!"};
        if (! done[parent[id]].load())
            throw std::runtime_error{"Parent has not been worked on!"};

        volatile size_t anti_opt = 0;
        for (size_t i = 0; i < work_amount[id]; ++i)
        {
            anti_opt += 1;
        }

        done[id].store(1);
        return std::make_pair(&edges[nodes[id]], &edges[nodes[id+1]]);
    }

    inline size_t size() { return n; }

    bool evaluate()
    {
        for (size_t i = 0; i < n; ++i)
            if (!done[i].load()) throw std::runtime_error("Work is not finished!");
        return true;
    }

    void reset()
    {
        for (size_t i = 0; i < n; ++i)
            done[i].store(0);
    }

private:
    size_t n;
    std::unique_ptr<size_t[]> nodes;
    std::unique_ptr<size_t[]> work_amount;
    std::unique_ptr<NodeId[]> edges;
    std::unique_ptr<NodeId[]> parent;
    std::unique_ptr<std::atomic_char[]> done;
};
