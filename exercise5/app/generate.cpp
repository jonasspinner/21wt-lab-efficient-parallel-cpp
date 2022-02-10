#include "instance_generation.h"

int main() {
    float p = 1.0;
    float q = 0.0;

    std::size_t num_elements = 1 << 3;
    std::size_t num_queries = 1 << 3;

    {
    auto b = epcpp::find_and_modifiy_benchmark(p, q);
    auto [setup, queries] = b.generate(num_elements, num_queries);

    std::cout << "# " << b.name() << "\n";
    for (auto op : setup) {
        std::cout << op << "\n";
    }
    std::cout << "---\n";
    for (auto op : queries) {
        std::cout << op << "\n";
    }
    }

    {
        auto b = epcpp::find_benchmark(p);
        auto [setup, queries] = b.generate(num_elements, num_queries);

        std::cout << "# " << b.name() << "\n";
        for (auto op : setup) {
            std::cout << op << "\n";
        }
        std::cout << "---\n";
        for (auto op : queries) {
            std::cout << op << "\n";
        }
    }

    {
        auto b = epcpp::successful_find_benchmark();
        auto [setup, queries] = b.generate(num_elements, num_queries);

        std::cout << "# " << b.name() << "\n";
        for (auto op : setup) {
            std::cout << op << "\n";
        }
        std::cout << "---\n";
        for (auto op : queries) {
            std::cout << op << "\n";
        }
    }


    return 0;
}
