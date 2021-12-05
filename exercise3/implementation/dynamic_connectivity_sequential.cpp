#include "dynamic_connectivity_sequential.hpp"
#include "adj_array.hpp"

DynamicConnectivity::DynamicConnectivity(long num_nodes) : union_find_parents(num_nodes), union_find_ranks(num_nodes, 0)
{
    if (num_nodes >= std::numeric_limits<Node>::max())
        throw std::runtime_error("Node type to small");

    for (Node u = 0; u < num_nodes; ++u) {
        union_find_parents[u] = u;
    }
}



void DynamicConnectivity::addEdges(const EdgeList& edges)
{
    for (const auto &e : edges) {
        if (unite(e.from, e.to)) {
            // it's necessary to include both directions, because AdjacencyArray is directed
            filtered_edges.push_back(e);
            filtered_edges.push_back({e.to, e.from, e.length});
        }
    }

    AdjacencyArrayT<Node> graph(union_find_parents.size(), filtered_edges);
    bfs(graph);
}



bool DynamicConnectivity::connected(Node a, Node b) const
{
    return find_representative(a) == find_representative(b);
}


// Must return -1 for roots.
DynamicConnectivity::Node DynamicConnectivity::parentOf(Node n) const
{
    return bfs_parents[n];
}


DynamicConnectivity::Node DynamicConnectivity::find_representative_and_compress(Node node)
{
    Node root = find_representative(node);

    while (union_find_parents[node] != root) {
        Node parent = union_find_parents[node];
        union_find_parents[node] = root;
        node = parent;
    }

    return root;
}

DynamicConnectivity::Node DynamicConnectivity::find_representative(Node node) const
{
    Node root = node;
    while (union_find_parents[root] != root) {
        root = union_find_parents[root];
    }
    return root;
}

bool DynamicConnectivity::unite(Node a, Node b)
{
    a = find_representative_and_compress(a);
    b = find_representative_and_compress(b);
    if (a == b) {
        return false;
    }
    if (union_find_ranks[a] < union_find_ranks[b]) {
        std::swap(a, b);
    }
    union_find_parents[b] = a;
    if (union_find_ranks[a] == union_find_ranks[b]) {
        union_find_ranks[a] = union_find_ranks[a] + 1;
    }
    return true;
}

void DynamicConnectivity::bfs(const AdjacencyArrayT<Node> &graph) {
    bfs_frontier.reserve(graph.numNodes());
    bfs_next_frontier.reserve(graph.numNodes());
    bfs_parents.assign(graph.numNodes(), -1);
    bfs_visited.assign(graph.numNodes(), false);

    for (Node u = 0; u < graph.numNodes(); ++u) {
        assert(bfs_frontier.empty());
        assert(bfs_next_frontier.empty());
        if (union_find_parents[u] != u) {
            continue;
        }

        bfs_frontier.push_back(u);
        bfs_visited[graph.nodeId(u)] = true;

        while (!bfs_frontier.empty()) {
            while (!bfs_frontier.empty()) {
                Node n = bfs_frontier.back();
                bfs_frontier.pop_back();

                for (auto e = graph.beginEdges(n); e < graph.endEdges(n); ++e) {
                    auto neighbor = graph.edgeHead(e);
                    auto id = graph.nodeId(neighbor);
                    if (!bfs_visited[id]) {
                        bfs_visited[id] = true;
                        bfs_parents[neighbor] = n;
                        bfs_next_frontier.push_back(neighbor);
                    }
                }
            }
            std::swap(bfs_frontier, bfs_next_frontier);
        }

        assert(bfs_frontier.empty());
        assert(bfs_next_frontier.empty());
    }
}