#ifndef EXERCISE5_INSTANCE_GENERATION_H
#define EXERCISE5_INSTANCE_GENERATION_H

#include <vector>
#include <random>
#include <exception>
#include <iostream>

namespace epcpp {
    enum class OperationKind {
        Insert,
        Find,
        Remove
    };

    template<class Value>
    struct Operation {
        Operation(OperationKind kind, Value value) : kind(kind), value(value) {}
        OperationKind kind;
        Value value;
    };


    std::vector<Operation<int>>
    generate_list_int_instances(std::size_t num_operations, float p_insert, float p_find, float p_remove) {
        if (std::abs(1.0f - (p_insert + p_find + p_remove)) > 1e-4f) {
            throw std::invalid_argument("");
        }

        std::mt19937_64 gen;
        std::geometric_distribution<int> key_dist(0.4);
        std::bernoulli_distribution find_dist(p_find);
        std::bernoulli_distribution insert_vs_remove_dist(p_insert / (p_insert + p_remove));

        std::vector<Operation<int>> result;
        result.reserve(num_operations);

        for (std::size_t i = 0; i < num_operations; ++i) {
            int key = key_dist(gen);
            if (find_dist(gen)) {
                result.push_back(Operation<int>{OperationKind::Find, key});
            } else if (insert_vs_remove_dist(gen)) {
                result.push_back(Operation<int>{OperationKind::Insert, key});
            } else {
                result.push_back(Operation<int>{OperationKind::Remove, key});
            }
        }
    }


    std::vector<Operation<int>> generate_operations(std::size_t num_finds, std::size_t num_modifications) {
        std::mt19937_64 gen;
        std::geometric_distribution<int> key_dist(0.4);

        std::vector<Operation<int>> result;
        result.reserve(num_finds + num_modifications);
        for (std::size_t i = 0; i < num_finds; ++i) {
            int key = key_dist(gen);
            result.emplace_back(OperationKind::Find, key);
        }
        for (std::size_t i = 0; i < num_modifications / 2; ++i) {
            int key = key_dist(gen);
            result.emplace_back(OperationKind::Insert, key);
            result.emplace_back(OperationKind::Remove, key);
        }

        std::shuffle(result.begin(), result.end(), gen);

        return result;
    }
}

std::ostream &operator<<(std::ostream &os, epcpp::OperationKind kind) {
    using epcpp::OperationKind;
    if (kind == OperationKind::Find) {
        os << "Find";
    } else if (kind == OperationKind::Insert) {
        os << "Insert";
    } else if (kind == OperationKind::Remove) {
        os << "Remove";
    } else {
        os.setstate(std::ios_base::failbit);
    }
    return os;
}

std::istream &operator>>(std::istream &is, epcpp::OperationKind &kind) {
    using epcpp::OperationKind;
    std::string s;
    is >> s;
    if (s == "Find") {
        kind = OperationKind::Find;
    } else if (s == "Insert") {
        kind = OperationKind::Insert;
    } else if (s == "Remove") {
        kind = OperationKind::Remove;
    } else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

template <class Value>
std::ostream &operator<<(std::ostream &os, epcpp::Operation<Value> operation) {
    return os << operation.kind << " " << operation.value;
}

template <class Value>
std::istream &operator>>(std::istream &is, epcpp::Operation<Value> &operation) {
    return is >> operation.kind >> operation.value;
}

#endif //EXERCISE5_INSTANCE_GENERATION_H
