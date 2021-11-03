#include <gtest/gtest.h>

#include "implementation/edge_list.hpp"

#include "implementation/adj_array.hpp"
#include "implementation/adj_list.hpp"
#include "implementation/node_graph.hpp"
#include "implementation/weighted_graph_paired.hpp"
#include "implementation/weighted_graph_separated.hpp"

#include "implementation/bfs.hpp"
#include "implementation/dijkstra.hpp"


struct AdjArr
{
    static auto make(size_t n, const EdgeList& elist)
    {
        return AdjacencyArray(n,elist);
    }
};

struct AdjList
{
    static auto make(size_t n, const EdgeList& elist)
    {
        return AdjacencyList(n,elist);
    }
};

struct NGraph
{
    static auto make(size_t n, const EdgeList& elist)
    {
        return NodeGraph(n,elist);
    }
};

struct WPair
{
    static auto make(size_t n, const EdgeList& elist)
    {
        return WeightedGraphPaired(n,elist);
    }
};

struct WSep
{
    static auto make(size_t n, const EdgeList& elist)
    {
        return WeightedGraphSeparated(n,elist);
    }
};




template <class P>
class GraphClassTest : public ::testing::Test
{
protected:
    auto make(size_t n, const EdgeList& elist)
    {
        return P::make(n, elist);
    }
};



using MyTypes = ::testing::Types<AdjArr,AdjList,NGraph,WPair,WSep>;
TYPED_TEST_CASE(GraphClassTest, MyTypes);

TYPED_TEST(GraphClassTest, construction)
{
    auto [elist, n] = readEdges("../data/test_graph.graph");
    auto g = this->make(n, elist);

    ASSERT_EQ(g.numNodes(), 43);

    ASSERT_EQ(g.beginEdges(g.node(23)), g.endEdges(g.node(23)));

    ASSERT_EQ(g.nodeId(g.edgeHead(g.beginEdges(g.node(20)))), 33);

    auto n42 = std::vector<size_t>();
    auto n42_precomp = std::vector<size_t>{1,7,12,14,15,21,29,30,33,36};

    for (auto e = g.beginEdges(g.node(42)); e < g.endEdges(g.node(42)); ++e)
    {
        n42.push_back(g.nodeId(g.edgeHead(e)));
    }
    ASSERT_EQ (n42.size(), n42_precomp.size());

    std::sort(n42.begin(), n42.end());
    for (size_t i = 0; i < n42_precomp.size(); ++i)
    {
        ASSERT_EQ (n42[i], n42_precomp[i]);
    }
}

TYPED_TEST(GraphClassTest, bfs_test)
{
    auto [elist, n] = readEdges("../data/test_graph.graph");
    auto g = this->make(n, elist);

    auto bfsh = BFSHelper<decltype(g)>(g);

    ASSERT_TRUE(bfsh.bfs(g.node(4),  g.node(42)) >= g.numNodes());
    ASSERT_EQ(bfsh.bfs(g.node(26), g.node(26)), 0);
    ASSERT_EQ(bfsh.bfs(g.node( 1), g.node( 2)), 1);
    ASSERT_EQ(bfsh.bfs(g.node(18), g.node(32)), 4);
}


using MyTypesWeighted = ::testing::Types<WPair,WSep>;
TYPED_TEST_CASE(WeightedGraphClassTest, MyTypesWeighted);

template <class P>
class WeightedGraphClassTest : public ::testing::Test
{
protected:
    auto make(size_t n, const EdgeList& elist)
    {
        return P::make(n, elist);
    }
};

TYPED_TEST(WeightedGraphClassTest, dijkstra_test)
{
    auto [elist, n] = readEdges("../data/test_graph.graph");
    auto g = this->make(n, elist);

    auto dijh = DijkstraHelper<decltype(g)>(g);

    ASSERT_TRUE(dijh.dijkstra(g.node(4),  g.node(42)) >= 999.);
    ASSERT_DOUBLE_EQ  (dijh.dijkstra(g.node(26), g.node(26)), 0.);

    ASSERT_DOUBLE_EQ  (dijh.dijkstra(g.node( 1), g.node( 2)), 1.95864);
    ASSERT_DOUBLE_EQ  (dijh.dijkstra(g.node(18), g.node(32)), 6.0892299999999997);
}
