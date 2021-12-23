#include "reference_solution.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

ReferenceSolution::ReferenceSolution(std::size_t num_nodes)
    : union_find_(num_nodes)
    , parent_of_(num_nodes, no_parent)
{
    assert(num_nodes <= std::numeric_limits<Node>::max());
    assert(num_nodes <= (std::numeric_limits<EdgeIndex>::max() - 1) / 2);
    edges_.reserve(num_nodes);
}

void ReferenceSolution::addEdges(const EdgeList& edges) {
    std::atomic_size_t num_edges(edges_.size());
    // Make sure there's enough space.
    edges_.resize(numNodes());
    #pragma omp parallel
    {
        // Avoid contention on shared data.
        std::vector<Edge> my_edges;
        #pragma omp for
        for (auto& edge : edges) {
            const Edge my_edge{static_cast<Node>(edge.from), static_cast<Node>(edge.to)};
            if (unite(my_edge)) {
                // Found a new tree edge.
                my_edges.push_back(my_edge);
            }
        }
        // Copy our local edges to the shared data.
        const auto i = num_edges.fetch_add(my_edges.size(), std::memory_order_relaxed);
        std::copy(my_edges.begin(), my_edges.end(),
                  edges_.begin() + static_cast<std::ptrdiff_t>(i));
    }
    // Discard the extra reserved space.
    edges_.resize(num_edges);

    rebuild_parent_array();
}

auto ReferenceSolution::find_representative(Node a) const -> RootAndRank {
    auto parent_or_rank = union_find_[a].load(std::memory_order_relaxed);
    // A non-positive value represents the rank of this root.
    if (parent_or_rank <= 0) return {a, parent_or_rank};

    // Otherwise, the value is the parent of this node + 1.
    // This offset avoids extra initialization in the constructor.
    const auto parent = static_cast<Node>(parent_or_rank) - 1;
    const auto root_and_rank = find_representative(parent);
    // Apply path compression.
    if (parent != root_and_rank.root) {
        union_find_[a].compare_exchange_strong(parent_or_rank,
                                               static_cast<NodeOrRank>(root_and_rank.root + 1),
                                               std::memory_order_relaxed);
    }
    return root_and_rank;
}

bool ReferenceSolution::link(RootAndRank parent, RootAndRank child) {
    return union_find_[child.root].compare_exchange_strong(
            child.rank, static_cast<NodeOrRank>(parent.root + 1), std::memory_order_relaxed);
}

void ReferenceSolution::increase_rank(RootAndRank root) {
    // Increasing the rank might fail due to concurrent updates,
    // but it should be rare and doesn't matter for correctness.
    union_find_[root.root].compare_exchange_strong(root.rank, root.rank - 1,
                                                   std::memory_order_relaxed);
}

bool ReferenceSolution::unite(Edge edge) {
    auto from = find_representative(edge.from);
    auto to = find_representative(edge.to);

    while (from.root != to.root) {
        // Do union by rank. Rank is stored as a negative value, so the
        // comparisons here are inverted from what one might expect.
        if (from.rank < to.rank) {
            if (link(from, to)) return true;
        } else if (to.rank < from.rank) {
            if (link(to, from)) return true;
        // If both roots have the same rank, break the tie consistently, to avoid creating cycles.
        } else if (to.root < from.root) {
            if (link(from, to)) {
                increase_rank(from);
                return true;
            }
        } else {
            if (link(to, from)) {
                increase_rank(to);
                return true;
            }
        }
        // If we failed to update the union find data, someone else did a concurrent update.
        from = find_representative(from.root);
        to = find_representative(to.root);
    }
    return false;
}

void ReferenceSolution::Graph::build(const ReferenceSolution& dc) {
    // Can't resize a vector of atomics, because atomics can't be moved.
    if (index_.size() < dc.numNodes() + 1)
        index_ = std::vector<std::atomic<EdgeIndex>>(dc.numNodes() + 1);
    // We need undirected edges.
    edges_.resize(2 * dc.numEdges());

    #pragma omp parallel
    {
        // Reset index.
        #pragma omp for
        for (auto& i : index_) {
            i.store(0, std::memory_order_relaxed);
        }
        // Compute degrees.
        #pragma omp for
        for (auto& edge : dc.edges()) {
            index_[edge.from].fetch_add(1, std::memory_order_relaxed);
            index_[edge.to].fetch_add(1, std::memory_order_relaxed);
        }
        // Compute prefix sum.
        #pragma omp single
        {
            EdgeIndex sum = 0;
            for (std::size_t i = 0; i < dc.numNodes(); ++i) {
                sum += index_[i].load(std::memory_order_relaxed);
                index_[i].store(sum, std::memory_order_relaxed);
            }
            index_[dc.numNodes()].store(sum, std::memory_order_relaxed);
        }
        // Store edges.
        #pragma omp for
        for (auto& edge : dc.edges()) {
            auto idx = index_[edge.from].fetch_sub(1, std::memory_order_relaxed) - 1;
            edges_[idx] = edge.to;
            idx = index_[edge.to].fetch_sub(1, std::memory_order_relaxed) - 1;
            edges_[idx] = edge.from;
        }
    }
}

template <class F>
void ReferenceSolution::Graph::for_each_neighbor_of(Node node, F&& func) const {
    const auto end = index_[node + 1].load(std::memory_order_relaxed);
    for (auto i = index_[node].load(std::memory_order_relaxed); i != end; ++i) {
        func(edges_[i]);
    }
}

void ReferenceSolution::rebuild_parent_array() {
    graph_.build(*this);

    std::vector<Node> roots;
    for (Node n = 0; n < numNodes(); ++n)
        if (union_find_[n] <= 0) roots.push_back(n);

    // This is a DFS, not a BFS, but it doesn't matter here.
    std::vector<Edge> queue;
    #pragma omp parallel for private(queue)
    for (Node root : roots) {
        parent_of_[root] = no_parent;
        graph_.for_each_neighbor_of(root, [&](Node n) {
            queue.push_back({root, n});
        });
        while (!queue.empty()) {
            const auto [parent, node] = queue.back();
            queue.pop_back();
            parent_of_[node] = parent;
            graph_.for_each_neighbor_of(node, [&](Node n) {
                // Don't go back.
                if (n != parent)
                    queue.push_back({node, n});
            });
        }
    }
}
