#pragma once

#include <atomic>
#include <list>
#include <thread>
#include <vector>

#include "main.hpp"

namespace parallel_bfs {
namespace impl {

enum VertColor { WHITE, GRAY, BLACK };

struct ThreadData {
    VertIdx_t idx;
    std::list<VertIdx_t> adj_list;
    bool is_done;
};

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
namespace unlimited_threads {
template <typename GraphType, typename VisitorType>
static void _traverse_vert(const GraphType &G, ThreadData &data,
                           VisitorType &visitor,
                           std::vector<AtomicWrapper<VertColor>> &visited)
{
    visitor.examine_vertex(data.idx, G);

    const auto &edges = boost::out_edges(data.idx, G);
    for (auto i = edges.first; i != edges.second; i++) {
        visitor.examine_edge(*i, G);
        VertIdx_t adj_idx = boost::target(*i, G);
        VertColor expected = WHITE;
        if (visited[adj_idx].compare_exchange_strong(expected, GRAY)) {
            visitor.tree_edge(*i, G);
            visitor.discover_vertex(adj_idx, G);
            data.adj_list.push_back(adj_idx);
        } else if (visited[adj_idx].load() == GRAY) {
            visitor.non_tree_edge(*i, G);
            visitor.gray_target(*i, G);
        } else {
            visitor.non_tree_edge(*i, G);
            visitor.black_target(*i, G);
        }
    }

    visited[data.idx] = BLACK;
    visitor.finish_vertex(data.idx, G);
    data.is_done = true;
}

template <typename GraphType, typename VisitorType>
void _breadth_first_search(const GraphType &G, VertIdx_t start,
                           VisitorType &visitor)
{
    std::vector<AtomicWrapper<VertColor>> visited(boost::num_vertices(G),
                                                  WHITE);

    auto vert_pair = boost::vertices(G);
    for (auto i = vert_pair.first; i != vert_pair.second; i++) {
        visitor.initialize_vertex(*i, G);
    }

    std::list<VertIdx_t> curr_lvl;
    std::list<ThreadData> next_lvl;
    visited[start] = GRAY;
    visitor.discover_vertex(start, G);
    curr_lvl.push_back(start);
    do {
        std::list<std::thread> thread_list;
        for (VertIdx_t vert_idx : curr_lvl) {
            next_lvl.push_back({.idx = vert_idx,
                                .adj_list = std::list<VertIdx_t>(),
                                .is_done = false});
            thread_list.push_back(
                std::thread(_traverse_vert<GraphType, VisitorType>, std::ref(G),
                            std::ref(next_lvl.back()), std::ref(visitor),
                            std::ref(visited)));
        }

        curr_lvl.clear();
        while (!next_lvl.empty()) { // Waiting
            auto next_lvl_i = next_lvl.begin();
            while (next_lvl_i != next_lvl.end()) {
                auto del_ref = next_lvl_i;
                if (next_lvl_i->is_done) {
                    curr_lvl.splice(curr_lvl.end(), next_lvl_i->adj_list);
                }
                next_lvl_i++; // increment before deleting
                if (del_ref->is_done) {
                    next_lvl.erase(del_ref);
                }
            }
        }

        for (std::thread &t : thread_list) {
            t.join();
        }
    } while (!curr_lvl.empty());
}
} // namespace unlimited_threads

namespace unlimited_threads_old {
template <typename GraphType, typename VisitorType>
static void _traverse_vert(const GraphType &G, VertIdx_t idx,
                           VisitorType &visitor,
                           std::list<VertIdx_t> &next_idxs,
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
        } else if (visited[adj_idx].load() == GRAY) {
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
void _breadth_first_search(const GraphType &G, VertIdx_t start,
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
            curr_lvl.splice(curr_lvl.end(), branch);
        }
        next_lvl.clear();
        std::list<std::thread> thread_list;
        for (VertIdx_t vert_idx : curr_lvl) {
            next_lvl.push_back(std::list<VertIdx_t>());
            thread_list.push_back(
                std::thread(_traverse_vert<GraphType, VisitorType>, std::ref(G),
                            vert_idx, std::ref(visitor),
                            std::ref(next_lvl.back()), std::ref(visited)));
        }

        // Waiting
        for (std::thread &t : thread_list) {
            t.join();
        }
    } while (!curr_lvl.empty());
}
} // namespace unlimited_threads_old

namespace fixed_thread_count {

template <typename GraphType, typename VisitorType>
static void _traverse_vert(const GraphType &G, ThreadData &data,
                           VisitorType &visitor,
                           std::vector<AtomicWrapper<VertColor>> &visited,
                           std::vector<std::size_t> &depth)
{
    visitor.examine_vertex(data.idx, G);

    const auto &edges = boost::out_edges(data.idx, G);
    for (auto i = edges.first; i != edges.second; i++) {
        visitor.examine_edge(*i, G);
        VertIdx_t adj_idx = boost::target(*i, G);
        VertColor expected = WHITE;
        if (visited[adj_idx].compare_exchange_strong(expected, GRAY)) {
            visitor.tree_edge(*i, G);
            visitor.discover_vertex(adj_idx, G);
            data.adj_list.push_back(adj_idx); //
            depth[adj_idx] = depth[data.idx] + 1;
        } else if (visited[adj_idx].load() == GRAY) {
            visitor.non_tree_edge(*i, G);
            visitor.gray_target(*i, G);
        } else {
            visitor.non_tree_edge(*i, G);
            visitor.black_target(*i, G);
        }
    }

    visited[data.idx] = BLACK;
    visitor.finish_vertex(data.idx, G);
    data.is_done = true; //
}

template <std::size_t THREAD_CNT, typename GraphType, typename VisitorType>
static void _breadth_first_search(const GraphType &G, VertIdx_t start,
                                  VisitorType &visitor)
{
    std::vector<AtomicWrapper<VertColor>> visited(boost::num_vertices(G),
                                                  WHITE);
    std::vector<std::size_t> depth(boost::num_vertices(G), 0);

    auto vert_pair = boost::vertices(G);
    for (auto i = vert_pair.first; i != vert_pair.second; i++) {
        visitor.initialize_vertex(*i, G);
    }

    std::list<VertIdx_t> queue;
    typename std::array<std::thread, THREAD_CNT> threads;
    typename std::array<ThreadData, THREAD_CNT> data;
    std::size_t busy_count = 0;
    std::size_t curr_depth = 0;
    data.fill({.is_done = true});

    visited[start] = GRAY;
    depth[start] = 0;
    visitor.discover_vertex(start, G);
    queue.push_back(start);
    do {
        std::size_t depth_limit =
            (busy_count == 0) ? curr_depth + 1 : curr_depth;
        for (std::size_t i = 0; !queue.empty() && i < THREAD_CNT; i++) {
            VertIdx_t vert_idx = queue.front();
            if (!threads[i].joinable() && depth[vert_idx] <= depth_limit) {
                queue.pop_front();
                data[i] = {.idx = vert_idx,
                           .adj_list = std::list<VertIdx_t>(),
                           .is_done = false};
                threads[i] = std::thread(_traverse_vert<GraphType, VisitorType>,
                                         std::ref(G), std::ref(data[i]),
                                         std::ref(visitor), std::ref(visited),
                                         std::ref(depth));
                busy_count++;
                curr_depth = depth[vert_idx];
            }
        }
        do {
            for (std::size_t i = 0; i < THREAD_CNT; i++) {
                if (threads[i].joinable() && data[i].is_done) {
                    queue.splice(queue.end(), data[i].adj_list);
                    threads[i].join();
                    busy_count--;
                }
            }
        } while (busy_count == THREAD_CNT || (queue.empty() && busy_count > 0));
    } while (!queue.empty());
}
} // namespace fixed_thread_count
} // namespace impl

enum ThreadCountOpt { UNLIMITED_THREADS = 0 };

template <std::size_t THREAD_CNT, typename GraphType, typename VisitorType>
void breadth_first_search(const GraphType &G, VertIdx_t start,
                          VisitorType &visitor)
{
    switch (THREAD_CNT) {
    case UNLIMITED_THREADS:
        impl::unlimited_threads::_breadth_first_search(G, start, visitor);
        break;

    default:
        impl::fixed_thread_count::_breadth_first_search<THREAD_CNT>(G, start,
                                                                    visitor);
        break;
    }
}
} // namespace parallel_bfs
