#include <Optimizer/ExecutionGraph.hpp>
#include <boost/graph/graphviz.hpp>

using namespace NES;

ExecutionNodePtr ExecutionGraph::getRoot() {
    executionVertex_iterator vi, vi_end, next;
    boost::tie(vi, vi_end) = vertices(graph);
    for (next = vi; vi != vi_end; vi = next) {
        ++next;
        if (graph[*vi].ptr->getId() == 0) {
            return graph[*vi].ptr;
        }
    }
    return nullptr;
};

bool ExecutionGraph::hasVertex(int search_id) const {
    // build vertex iterator
    executionVertex_iterator vertex, vertex_end, next_vertex;
    boost::tie(vertex, vertex_end) = vertices(graph);

    // iterator over vertices
    for (next_vertex = vertex; vertex != vertex_end; vertex = next_vertex) {
        ++next_vertex;

        // check for matching vertex
        if (graph[*vertex].id == search_id) {
            return true;
        }
    }

    return false;
};

bool ExecutionGraph::addVertex(ExecutionNodePtr ptr) {
    // does graph already contain vertex?
    if (hasVertex(ptr->getId())) {
        return false;
    }

    // add vertex
    boost::add_vertex(ExecutionVertex{ptr->getId(), ptr}, graph);
    return true;
};

vector<ExecutionVertex> ExecutionGraph::getAllVertex() const {

    vector<ExecutionVertex> result = {};

    executionVertex_iterator vertex, vertex_end, next_vertex;
    boost::tie(vertex, vertex_end) = vertices(graph);

    // iterator over vertices
    for (next_vertex = vertex; vertex != vertex_end; vertex = next_vertex) {
        ++next_vertex;
        result.push_back(graph[*vertex]);
    }

    return result;
}

bool ExecutionGraph::removeVertex(int search_id) {
    // does graph contain vertex?
    if (hasVertex(search_id)) {
        remove_vertex(getVertex(search_id), graph);
        return true;
    }

    return false;
};

const executionVertex_t ExecutionGraph::getVertex(int search_id) const {
    assert(hasVertex(search_id));

    // build vertex iterator
    executionVertex_iterator vertex, vertex_end, next_vertex;
    boost::tie(vertex, vertex_end) = vertices(graph);

    // iterator over vertices
    for (next_vertex = vertex; vertex != vertex_end; vertex = next_vertex) {
        ++next_vertex;

        // check for matching vertex
        if (graph[*vertex].id == search_id) {
            return *vertex;
        }
    }

    // should never happen
    return executionVertex_t();
};

const ExecutionNodePtr ExecutionGraph::getNode(int search_id) const {
    // build vertex iterator
    executionVertex_iterator vertex, vertex_end, next_vertex;
    boost::tie(vertex, vertex_end) = vertices(graph);

    // iterator over vertices
    for (next_vertex = vertex; vertex != vertex_end; vertex = next_vertex) {
        ++next_vertex;

        // check for matching vertex
        if (graph[*vertex].id == search_id) {
            return graph[*vertex].ptr;
        }
    }

    // should never happen
    return nullptr;
}

ExecutionNodeLinkPtr ExecutionGraph::getLink(ExecutionNodePtr sourceNode, ExecutionNodePtr destNode) const {
    // build edge iterator
    boost::graph_traits<executionGraph_t>::edge_iterator edge, edge_end, next_edge;
    boost::tie(edge, edge_end) = edges(graph);

    // iterate over edges and check for matching links
    for (next_edge = edge; edge != edge_end; edge = next_edge) {
        ++next_edge;

        // check, if link does match
        auto current_link = graph[*edge].ptr;
        if (current_link->getSource()->getId() == sourceNode->getId() &&
            current_link->getDestination()->getId() == destNode->getId()) {
            return current_link;
        }
    }

    // no edge matched
    return nullptr;
};

bool ExecutionGraph::hasLink(ExecutionNodePtr sourceNode, ExecutionNodePtr destNode) const {
    // edge found
    if (getLink(sourceNode, destNode) != nullptr) {
        return true;
    }

    // no edge found
    return false;
};

const ExecutionEdge* ExecutionGraph::getEdge(int search_id) const {
    // build edge iterator
    executionEdge_iterator edge, edge_end, next_edge;
    boost::tie(edge, edge_end) = edges(graph);

    // iterate over edges
    for (next_edge = edge; edge != edge_end; edge = next_edge) {
        ++next_edge;

        // return matching edge
        if (graph[*edge].id == search_id) {
            return &graph[*edge];
        }
    }

    // no matching edge found
    return nullptr;
};

vector<ExecutionEdge> ExecutionGraph::getAllEdges() const {

    vector<ExecutionEdge> result = {};

    executionEdge_iterator edge, edge_end, next_edge;
    boost::tie(edge, edge_end) = edges(graph);

    // iterate over edges
    for (next_edge = edge; edge != edge_end; edge = next_edge) {
        ++next_edge;
        result.push_back(graph[*edge]);
    }

    return result;
}

bool ExecutionGraph::hasEdge(int search_id) const {
    // edge found
    if (getEdge(search_id) != nullptr) {
        return true;
    }

    // no edge found
    return false;
};

bool ExecutionGraph::addEdge(ExecutionNodeLinkPtr ptr) {
    // link id is already in graph
    if (hasEdge(ptr->getLinkId())) {
        return false;
    }

    // there is already a link between those two vertices
    if (hasLink(ptr->getSource(), ptr->getDestination())) {
        return false;
    }

    // check if vertices are in graph
    if (!hasVertex(ptr->getSource()->getId()) || !hasVertex(ptr->getDestination()->getId())) {
        return false;
    }

    // add edge with link
    auto src = getVertex(ptr->getSource()->getId());
    auto dst = getVertex(ptr->getDestination()->getId());
    boost::add_edge(src, dst, ExecutionEdge{ptr->getLinkId(), ptr}, graph);

    return true;
};

const std::vector<ExecutionEdge> ExecutionGraph::getAllEdgesToNode(ExecutionNodePtr destNode) const {

    std::vector<ExecutionEdge> result = {};

    executionEdge_iterator edge, edge_end, next_edge;
    boost::tie(edge, edge_end) = edges(graph);

    for (next_edge = edge; edge != edge_end; edge = next_edge) {
        ++next_edge;

        if (graph[*edge].ptr->getDestination()->getId() == destNode.get()->getId()) {
            result.push_back(graph[*edge]);
        }
    }

    return result;
}

const vector<ExecutionEdge> ExecutionGraph::getAllEdgesFromNode(NES::ExecutionNodePtr srcNode) const {
    std::vector<ExecutionEdge> result = {};

    executionEdge_iterator edge, edge_end, next_edge;
    boost::tie(edge, edge_end) = edges(graph);

    for (next_edge = edge; edge != edge_end; edge = next_edge) {
        ++next_edge;

        if (graph[*edge].ptr->getSource()->getId() == srcNode.get()->getId()) {
            result.push_back(graph[*edge]);
        }
    }

    return result;
}

std::string ExecutionGraph::getGraphString() {
    std::stringstream ss;
    boost::write_graphviz(ss, graph,
                          [&](auto& out, auto v) {
                            out << "[label=\"" << graph[v].id << " operatorName="
                                << graph[v].ptr->getOperatorName() << " nodeName="
                                << graph[v].ptr->getNodeName()
                                << "\"]";
                          },
                          [&](auto& out, auto e) { out << "[label=\"" << graph[e].id << "\"]"; });
    ss << std::flush;
    return ss.str();
}
