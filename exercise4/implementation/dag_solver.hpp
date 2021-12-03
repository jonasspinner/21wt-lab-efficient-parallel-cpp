#pragma once

#include "dag.hpp"

// This template is beneficial for our consistent interface
template <class DAG = DAGTask>
class DAGSolver
{
public:
    DAGSolver(DAG& dag) : _dag(dag)
    {
        // Here you could do some precomputation (i.e. explore the graph)
    }
    DAGSolver(const DAGSolver& src) = delete;
    DAGSolver(DAGSolver&& src) = default;
    DAGSolver& operator=(const DAGSolver& src) = delete;
    DAGSolver& operator=(DAGSolver&& src)
    {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src) return *this;
        this->~DAGSolver();
        new (this) DAGSolver(std::move(src));
        return *this;
    }


    void solve()
    {
        // TODO
    }

    void reset()
    {
        _dag.reset();
        // TODO reset your own data structures
    }

private:
    DAG& _dag;
    // TODO some additional data structures
};
