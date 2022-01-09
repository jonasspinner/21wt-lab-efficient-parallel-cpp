#include <iostream>
#include <string>

#include "utils/commandline.h"

#include "implementation/tree.hpp"
#include "implementation/tree_solver.hpp"


int main(int argn, char** argc)
{
    CommandLine cl(argn, argc);

    std::string file = cl.strArg("-file", "../data/tree_10000.graph");

    TreeTask             tree(file, 0.1);
    TreeSolver<TreeTask> solver(tree);

    for (size_t i = 0; i < 5; ++i)
    {
        solver.solve();
        if (tree.evaluate()) std::cout << i << " tree-test successful!\n";
        solver.reset();
    }

    return 0;
}
