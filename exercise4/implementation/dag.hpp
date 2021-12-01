#pragma once

#include <cstddef>
#include <fstream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

// DO NOT CHANGE THIS

class DAGTask
{

public:
    using NodeId       = uint;
    using NodeIterator = uint*;

    DAGTask(std::string file, double work_factor = 1)
    {
        std::ifstream in(file);
        in >> n; in >> e;

        nodes         = std::make_unique<size_t[]>(n+1);
        reverse_nodes = std::make_unique<size_t[]>(n+1);
        work_amount   = std::make_unique<size_t[]>(n);
        edges         = std::make_unique<NodeId[]>(e);
        parent        = std::make_unique<NodeId[]>(e);
        done          = std::make_unique<std::atomic_char[]>(n);

        for (size_t i = 0; i<n; ++i)
        {
            reverse_nodes[i] = 0;
            done[i].store(0);
        }
        reverse_nodes[n] = 0;

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
                reverse_nodes[temp]++;
            }

            in >> work;
            work_amount[ni] = work * work_factor;
        }

        for (size_t ni = 0; ni < n; ++ni)
            reverse_nodes[ni+1] += reverse_nodes[ni];

        nodes[n] = e;
        reverse_nodes[n] = e;

        for (size_t ni = 0; ni < n; ++ni)
            for (size_t ne = nodes[ni]; ne < nodes[ni+1]; ++ne)
                parent[--reverse_nodes[edges[ne]]] = ni;
    }

    // returns the start and the end iterator for the child nodes
    void work(NodeId id)
    {
        if (done[id].load()) throw std::runtime_error{"Already worked on this node!"};
        for (size_t i = reverse_nodes[id]; i < reverse_nodes[id+1]; ++i)
            if (! done[parent[i]].load())
            {
                throw std::runtime_error{"Parent has not been worked on!"};
            }

        volatile size_t anti_opt = 0;
        for (size_t i = 0; i < work_amount[id]; ++i)
        {
            anti_opt += 1;
        }

        done[id].store(1);
    }

    std::pair<NodeIterator, NodeIterator> outgoing(NodeId id)
    {
        return std::make_pair(&edges[nodes[id]],
                              &edges[nodes[id + 1]]);
    }

    std::pair<NodeIterator, NodeIterator> incoming(NodeId id)
    {
        return std::make_pair(&parent[reverse_nodes[id]],
                              &parent[reverse_nodes[id + 1]]);
    }

    size_t size() { return n; }

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

    bool test_graph_structure(NodeId id)
    {
        auto [in_begin, in_end] = incoming(id);
        for (NodeIterator curr = in_begin; curr != in_end; ++curr)
        {
            NodeId prev = *curr;
            auto [out_begin, out_end] = outgoing(prev);
            bool works = false;
            for (NodeIterator out = out_begin; out != out_end; ++out)
            {
                if (*out == id) works = true;
            }
            if (!works) return false;
        }
        return true;
    }

private:
    // necessary
    size_t n;
    size_t e;

    std::unique_ptr<size_t[]> nodes;
    std::unique_ptr<size_t[]> reverse_nodes;
    std::unique_ptr<size_t[]> work_amount;
    std::unique_ptr<NodeId[]> edges;
    std::unique_ptr<NodeId[]> parent;

    std::unique_ptr<std::atomic_char[]> done;

};
