#include <iostream>
#include <string>

#include "utils/commandline.h"

#include "implementation/dag.hpp"
#include "implementation/dag_solver.hpp"


int main(int argn, char** argc)
{
    CommandLine cl(argn, argc);

    std::string file = cl.strArg("-file", "../data/graph_100.graph");

    DAGTask            dag(file, 0.1);
    DAGSolver<DAGTask> solver(dag);

    for (size_t i = 0; i < 5; ++i)
    {
        solver.solve();
        if (dag.evaluate()) std::cout << "DAG-test successful!" << std::endl;
        solver.reset();
    }

    return 0;
}
