#include "dynamic_connectivity.hpp"

DynamicConnectivity::DynamicConnectivity(long num_nodes)
{
}



void DynamicConnectivity::addEdges(const EdgeList& edges)
{
}



bool DynamicConnectivity::connected(Node a, Node b) const
{
    return false;
}


// Must return -1 for roots.
DynamicConnectivity::Node DynamicConnectivity::parentOf(Node n) const
{
    return -1;
}


DynamicConnectivity::Node DynamicConnectivity::find_representative(Node a) const
{
    return a;
}
