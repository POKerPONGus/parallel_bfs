#pragma once

#include <queue>

#include "main.hpp"

namespace basic_bfs {

enum VertColor { WHITE, GRAY, BLACK };

template <class GraphType, class VisitorType>
void breadth_first_search(const GraphType &G, VertIdx_t start,
                              VisitorType &visitor)
{
    std::vector<VertColor> visited(boost::num_vertices(G), WHITE);
    std::queue<VertIdx_t> queue;

    auto vert_pair = boost::vertices(G);
    for (auto i = vert_pair.first; i != vert_pair.second; i++) {
        visitor.initialize_vertex(*i, G);
    }

    visited[start] = GRAY;
    visitor.discover_vertex(start, G);
    queue.push(start);

    while (!queue.empty()) {
        VertIdx_t idx = queue.front();
        queue.pop();
        visitor.examine_vertex(idx, G);

        const auto &edges = boost::out_edges(idx, G);
        for (auto i = edges.first; i != edges.second; i++) {
            visitor.examine_edge(*i, G);
            VertIdx_t adj_idx = boost::target(*i, G);
            if (visited[adj_idx] == WHITE) {
                visited[adj_idx] = GRAY;
                visitor.tree_edge(*i, G);
                visitor.discover_vertex(adj_idx, G);
                queue.push(adj_idx);
            } else if (visited[idx] == GRAY) {
                visitor.non_tree_edge(*i, G);
                visitor.gray_target(*i, G);
            } else {
                visitor.non_tree_edge(*i, G);
                visitor.black_target(*i, G);
            }
        }

        visited[idx] = BLACK;
        visitor.finish_vertex(idx, G);
    }
}
} // namespace my