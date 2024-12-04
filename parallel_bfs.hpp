#pragma once

#include <atomic>
#include <list>
#include <thread>
#include <vector>

#include "main.hpp"

namespace parallel {

template <typename T> class AtomicWrapper : public std::atomic<T> {
  public:
    AtomicWrapper() : std::atomic<T>() {}
    AtomicWrapper(const T &val) : std::atomic<T>(val) {}
    AtomicWrapper(const std::atomic<T> &val) : std::atomic<T>(val.load()) {}
    AtomicWrapper(const AtomicWrapper<T> &val) : std::atomic<T>(val.load()) {}
    AtomicWrapper &operator=(const AtomicWrapper<T> &val)
    {
        this->store(val.load());
        return *this;
    }

    AtomicWrapper &operator=(const T &value)
    {
        this->store(value);
        return *this;
    }
};

template <typename GraphType, typename VisitorType>
static void traverse_vert(const GraphType &G, VertIdx_t idx,
                          VisitorType &visitor, std::list<VertIdx_t> &next_idxs,
                          std::vector<AtomicWrapper<VertColor>> &visited)
{
    visitor.examine_vertex(idx, G);

    const auto &edges = boost::out_edges(idx, G);
    for (auto i = edges.first; i != edges.second; i++) {
        visitor.examine_edge(*i, G);
        VertIdx_t adj_idx = boost::target(*i, G);
        VertColor expected = WHITE;
        if (visited[adj_idx].compare_exchange_strong(expected, GRAY)) {
            visitor.tree_edge(*i, G);
            visitor.discover_vertex(adj_idx, G);
            next_idxs.push_back(adj_idx);
        } else if (visited[idx].load() == GRAY) {
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

template <typename GraphType, typename VisitorType>
void breadth_first_search(const GraphType &G, VertIdx_t start,
                          VisitorType &visitor)
{
    std::vector<AtomicWrapper<VertColor>> visited(boost::num_vertices(G),
                                                  WHITE);

    auto vert_pair = boost::vertices(G);
    for (auto i = vert_pair.first; i != vert_pair.second; i++) {
        visitor.initialize_vertex(*i, G);
    }

    std::list<VertIdx_t> curr_lvl;
    std::list<std::list<VertIdx_t>> next_lvl;
    visited[start] = GRAY;
    visitor.discover_vertex(start, G);
    next_lvl.push_back(std::list{start});
    do {
        curr_lvl.clear();
        for (std::list<VertIdx_t> &branch : next_lvl) {
            // for (auto e : branch)
            //     std::cout << e << ", ";
            // std::cout << "\n";

            curr_lvl.splice(curr_lvl.end(), branch);
            // std::cout << curr_lvl.size() << "\n";
            // for (auto e : curr_lvl)
            //     std::cout  << "-" << e << ", ";
            // std::cout << "\n";
        }
        // std::cout << "\n";
        next_lvl.clear();
        std::list<std::thread> thread_list(curr_lvl.size());
        auto thread_iter = thread_list.begin();
        for (VertIdx_t vert_idx : curr_lvl) {
            next_lvl.push_back(std::list<VertIdx_t>());
            *thread_iter =
                std::thread(traverse_vert<GraphType, VisitorType>, std::ref(G),
                            vert_idx, std::ref(visitor),
                            std::ref(next_lvl.back()), std::ref(visited));
            thread_iter++;
        }

        // Waiting
        for (std::thread &t : thread_list) {
            t.join();
        }
    } while (!curr_lvl.empty());
}
} // namespace parallel