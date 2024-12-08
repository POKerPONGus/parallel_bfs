#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>

#include "basic_bfs.hpp"
#include "graph_visitors.hpp"
#include "main.hpp"
#include "parallel_bfs.hpp"

static void _generate_graph(MyGraph_t &G, std::size_t vert_count,
                            std::size_t edge_rarity, std::uint32_t seed)
{
    for (int i = 0; i < vert_count; i++) {
        VertIdx_t v = boost::add_vertex(G);
        G[v].idx = i;
        G[v].name = "vertex no." + std::to_string(i);
    }
    std::srand(seed);
    std::random_device rd;
    std::mt19937 generator(rd());

    typedef boost::range_detail::integer_iterator<std::size_t> SizeIter_t;
    std::pair<SizeIter_t, SizeIter_t> vert_pair = boost::vertices(G);
    for (SizeIter_t i = vert_pair.first; i != vert_pair.second; i++) {

        std::vector<VertIdx_t> verts_vec(vert_pair.first, vert_pair.second);
        std::shuffle(verts_vec.begin(), verts_vec.end(), generator);

        for (VertIdx_t j : verts_vec) {
            if (std::rand() % (edge_rarity * boost::out_degree(*i, G) + 1) ==
                0) {
                boost::add_edge(*i, j, G);
                // std::cout << G[*i].name << ", " << G[j].name << "\n";
            }
        }
    }
}

typedef boost::iterator_property_map<
    std::vector<std::size_t>::iterator,
    boost::property_map<MyGraph_t, boost::vertex_index_t>::const_type>
    VertPMap_t;

#define BFS_IMPL_CNT 7

static std::array<BFSTimeVisitor<VertPMap_t>, BFS_IMPL_CNT>
_init_time_visitors(MyGraph_t &G,
                    std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> &vert_time)
{
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);

    std::array<VertPMap_t, BFS_IMPL_CNT> property_map;
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        property_map[i] = VertPMap_t(vert_time[i].begin(), vert_map);
    }
    std::array<BFSTimeVisitor<VertPMap_t>, BFS_IMPL_CNT> vis;
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        vis[i] = BFSTimeVisitor(property_map[i], 0);
    }
    return vis;
}

static std::array<BFSDistVisitor<VertPMap_t>, BFS_IMPL_CNT>
_init_dist_visitors(MyGraph_t &G,
                    std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> &vert_dist)
{
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);

    std::array<VertPMap_t, BFS_IMPL_CNT> property_map;
    std::array<BFSDistVisitor<VertPMap_t>, BFS_IMPL_CNT> vis;
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        property_map[i] = VertPMap_t(vert_dist[i].begin(), vert_map);
        vis[i] = BFSDistVisitor(property_map[i]);
    }
    return vis;
}

template <template <typename> class BFSVisitor>
static std::array<double, BFS_IMPL_CNT>
_test_bfs_impl(MyGraph_t &G, VertIdx_t start_idx,
               std::array<BFSVisitor<VertPMap_t>, BFS_IMPL_CNT> &vis)
{
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);

    Timer timer;
    std::array<double, BFS_IMPL_CNT> deltas;

    int idx = 0;
    timer.reset();
    boost::breadth_first_search(G, vert_map[start_idx],
                                boost::visitor(vis[idx]));
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    basic_bfs::breadth_first_search(G, vert_map[start_idx], vis[idx]);
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    parallel_bfs::breadth_first_search<parallel_bfs::UNLIMITED_THREADS>(
        G, vert_map[start_idx], vis[idx]);
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    parallel_bfs::breadth_first_search<2>(G, vert_map[start_idx], vis[idx]);
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    parallel_bfs::breadth_first_search<3>(G, vert_map[start_idx],
                                          vis[idx]); // incorrect result
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    parallel_bfs::breadth_first_search<4>(G, vert_map[start_idx], vis[idx]);
    deltas[idx] = timer.elapsed();
    idx++;

    timer.reset();
    parallel_bfs::breadth_first_search<5>(G, vert_map[start_idx], vis[idx]);
    deltas[idx] = timer.elapsed();
    idx++;

    std::cout << vert_map[start_idx] << "\n";
    std::vector<std::string> delta_strs(BFS_IMPL_CNT);
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        delta_strs[i] = std::to_string(deltas[i]);
    }
    std::cout << join_str(delta_strs, " vs ") << "\n";
    return deltas;
}

static std::array<bool, BFS_IMPL_CNT>
_comp_test_result(const std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> &attr,
                  std::string name)
{
    std::array<bool, BFS_IMPL_CNT> res;
    res[0] = true;
    for (int i = 1; i < BFS_IMPL_CNT; i++) {
        res[i] = (attr[0] == attr[i]) ? "true" : "false";
        std::cout << name << " 1 == " << i + 1 << ": "
                  << (res[i] ? "true" : "false") << "\n";
    }
    return res;
}

static void
_print_freq(const std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> &attr)
{
    std::array<std::map<std::size_t, VertIdx_t>, BFS_IMPL_CNT> attr_freqs;
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        attr_freqs[i] = get_freq_map(attr[i]);
    }
    for (auto freqs : attr_freqs) {
        for (auto kvp : freqs) {
            std::cout << kvp.first << ": " << kvp.second << ", ";
        }
        std::cout << "\n";
    }
}

static void _run(std::size_t vert_count, std::size_t edge_rarity,
                 std::string iter_id)
{
    std::uint32_t seed = std::time(0);
    MyGraph_t G;
    _generate_graph(G, vert_count, edge_rarity, seed);

    std::size_t start_idx = std::rand() % boost::num_vertices(G);
    std::cout << start_idx << "\n";

    std::cout << "\nDistance based test\n";
    std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> vert_dist;
    vert_dist.fill(std::vector<VertIdx_t>(boost::num_vertices(G)));
    auto dist_vistors = _init_dist_visitors(G, vert_dist);
    auto impl_time_on_dist =
        _test_bfs_impl<BFSDistVisitor>(G, start_idx, dist_vistors);
    auto dist_freq = get_freq_map(vert_dist[0]);
    auto dist_res_cmp = _comp_test_result(vert_dist, "dist");
    _print_freq(vert_dist);

    std::ofstream csv;
    std::vector<std::string> output_str;
    output_str.push_back(iter_id);
    output_str.push_back(std::to_string(vert_count));
    output_str.push_back(std::to_string(edge_rarity));
    std::size_t output_base_len = output_str.size();

    for (auto time : impl_time_on_dist) {
        output_str.push_back(std::to_string(time));
    }
    csv.open("impl_time_on_dist.csv", std::ios::app);
    csv << join_str(output_str, ", ") << "\n";
    csv.close();
    output_str.erase(output_str.begin() + output_base_len, output_str.end());

    for (auto kvp : dist_freq) {
        output_str.push_back(std::to_string(kvp.second));
    }
    csv.open("dist_freq.csv", std::ios::app);
    csv << join_str(output_str, ", ") << "\n";
    csv.close();
    output_str.erase(output_str.begin() + output_base_len, output_str.end());

    csv.open("wrong_results.csv", std::ios::app);
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        if (!dist_res_cmp[i]) {
            auto fail_dist_freq = get_freq_map(vert_dist[i]);
            output_str.push_back(std::to_string(seed));
            output_str.push_back(std::to_string(i));
            for (auto kvp : fail_dist_freq) {
                output_str.push_back(std::to_string(kvp.second));
            }
            csv << join_str(output_str, ", ") << "\n";
            output_str.erase(output_str.begin() + output_base_len,
                             output_str.end());
        }
    }
    csv.close();

    // std::cout << "\nTime based test\n";
    // std::array<std::vector<VertIdx_t>, BFS_IMPL_CNT> vert_time;
    // vert_time.fill(std::vector<VertIdx_t>(boost::num_vertices(G)));
    // auto time_vistors = _init_time_visitors(G, vert_time);
    // _test_bfs_impl<BFSTimeVisitor>(G, start_idx, time_vistors);
    // _comp_test_result(vert_time, "time");

    // boost::dynamic_properties attr;
    // attr.property("node_id", boost::get(boost::vertex_index, G));
    // attr.property("concentrate",
    //               boost::make_constant_property<MyGraph_t *>(true));

    // std::ofstream file("graph.dot");
    // boost::write_graphviz_dp(file, G, attr);
    // file.close();
}

int main()
{
    std::array<std::size_t, 10> vert_counts = {10,  20,   50,   100,  200,
                                               500, 1000, 2000, 5000, 10000};
    std::array<std::size_t, 9> edge_rarities = {1,  5,  10,  15, 20,
                                                30, 50, 100, 200};
#define START_I 78
#define END_I 100
    for (int i = START_I; i <= END_I; i++) {
        for (std::size_t count : vert_counts) {
            for (std::size_t rarity : edge_rarities) {
                _run(count, rarity, std::to_string(i));
                sleep(count/rarity);
            }
        }
    }
    return EXIT_SUCCESS;
}