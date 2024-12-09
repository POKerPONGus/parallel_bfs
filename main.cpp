
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>

#include <boost/graph/breadth_first_search.hpp>

#include "basic_bfs.hpp"
#include "graph_visitors.hpp"
#include "main.hpp"
#include "parallel_bfs.hpp"

static void _generate_graph(MyGraph_t &G, std::size_t vert_count,
                            std::size_t edge_rarity, std::uint32_t seed)
{
    for (std::size_t i = 0; i < vert_count; i++) {
        boost::add_vertex(G);
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
            }
        }
    }
}

enum GraphEdgeType { DIRECTED, UNDIRECTED };

template <GraphEdgeType EDGE_TYPE>
static void _load_graph(MyGraph_t &G, std::size_t vert_count,
                        std::string edges_file, std::string delim)
{
    std::ifstream file(edges_file);
    std::string buf;
    for (std::size_t i = 0; i < vert_count; i++) {
        boost::add_vertex(G);
    }

    while (std::getline(file, buf)) {
        std::vector<std::string> substrs = split_str(buf, delim);
        std::pair<std::size_t, std::size_t> edge;
        sscanf(substrs[0].c_str(), "%zu", &edge.first);
        sscanf(substrs[1].c_str(), "%zu", &edge.second);
        boost::add_edge(edge.first, edge.second, G);
        if (EDGE_TYPE == UNDIRECTED) {
            boost::add_edge(edge.second, edge.first, G);
        }
    }
}

typedef boost::iterator_property_map<
    std::vector<std::size_t>::iterator,
    boost::property_map<MyGraph_t, boost::vertex_index_t>::const_type>
    VertPMap_t;

#define BFS_IMPL_CNT 8

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

const std::vector<std::string> impl_names = {"boost",
                                             "basic",
                                             "new_unlimited_threads",
                                             "old_unlimited_threads",
                                             "2_threads",
                                             "3_threads",
                                             "4_threads",
                                             "5_threads"};

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
    parallel_bfs::impl::unlimited_threads_old::_breadth_first_search(
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

static void _run(MyGraph_t &G, std::vector<std::string> labels,
                 std::string file_prefix)
{
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
    std::string path;
    std::vector<std::string> output_str(labels.begin(), labels.end());
    std::size_t output_base_len = output_str.size();

    for (auto time : impl_time_on_dist) {
        output_str.push_back(std::to_string(time));
    }
    path = file_prefix + "impl_time_on_dist.csv";
    csv.open(path, std::ios::app);
    csv << join_str(output_str, ", ") << "\n";
    csv.close();
    output_str.erase(output_str.begin() + output_base_len, output_str.end());

    for (auto kvp : dist_freq) {
        output_str.push_back(std::to_string(kvp.second));
    }
    path = file_prefix + "dist_freq.csv";
    csv.open(path, std::ios::app);
    csv << join_str(output_str, ", ") << "\n";
    csv.close();
    output_str.erase(output_str.begin() + output_base_len, output_str.end());

    path = file_prefix + "wrong_results.csv";
    csv.open(path, std::ios::app);
    for (int i = 0; i < BFS_IMPL_CNT; i++) {
        if (!dist_res_cmp[i]) {
            auto fail_dist_freq = get_freq_map(vert_dist[i]);
            output_str.push_back(impl_names[i]);
            for (auto kvp : fail_dist_freq) {
                output_str.push_back(std::to_string(kvp.second));
            }
            csv << join_str(output_str, ", ") << "\n";
            output_str.erase(output_str.begin() + output_base_len,
                             output_str.end());
        }
    }
    csv.close();
}

void add_csv_header(std::vector<std::string> labels, std::string file_prefix)
{
    std::ofstream csv;
    std::string path;
    std::vector<std::string> output_str(labels.begin(), labels.end());
    std::size_t output_base_len = output_str.size();

    path = file_prefix + "impl_time_on_dist.csv";
    if (!std::filesystem::exists(path)) {
        output_str.insert(output_str.end(), impl_names.begin(),
                          impl_names.end());
        csv.open(path);
        csv << join_str(output_str, ", ") << "\n";
        csv.close();
        output_str.erase(output_str.begin() + output_base_len,
                         output_str.end());
    }

    path = file_prefix + "dist_freq.csv";
    if (!std::filesystem::exists(path)) {
        output_str.insert(output_str.end(), "levels...");
        csv.open(path);
        csv << join_str(output_str, ", ") << "\n";
        csv.close();
        output_str.erase(output_str.begin() + output_base_len,
                         output_str.end());
    }

    path = file_prefix + "wrong_results.csv";
    if (!std::filesystem::exists(path)) {
        output_str.insert(output_str.end(), {"impl_name", "levels..."});
        csv.open(path);
        csv << join_str(output_str, ", ") << "\n";
        csv.close();
        output_str.erase(output_str.begin() + output_base_len,
                         output_str.end());
    }
}

#define VER_COUNT_N 10
#define EDGE_RARITY_N 9

constexpr std::array<std::array<std::size_t, EDGE_RARITY_N>, VER_COUNT_N>
calc_rest_times(std::array<std::size_t, VER_COUNT_N> vert_counts,
                std::array<std::size_t, EDGE_RARITY_N> edge_rarities)
{
    auto rest_times =
        std::array<std::array<std::size_t, EDGE_RARITY_N>, VER_COUNT_N>();
    for (int i = 0; i < VER_COUNT_N; i++) {
        for (int j = 0; j < EDGE_RARITY_N; j++) {
            rest_times[i][j] = vert_counts[i] / edge_rarities[j] / 500;
        }
    }
    return rest_times;
}

#define START_I 1
#define END_I 1

int main()
{
    constexpr std::array<std::size_t, VER_COUNT_N> vert_counts = {
        10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
    constexpr std::array<std::size_t, EDGE_RARITY_N> edge_rarities = {
        1, 5, 10, 15, 20, 30, 50, 100, 200};
    constexpr auto rest_times = calc_rest_times(vert_counts, edge_rarities);

    std::string output_dir = "output/";
    std::filesystem::create_directory(output_dir);
    {
        std::string file_prefix = output_dir + "generated_";
        add_csv_header({"seed", "vert_count", "edge_count", "edge_rarity"},
                       file_prefix);
        for (int id = START_I; id <= END_I; id++) {
            for (int i = 0; i < VER_COUNT_N; i++) {
                for (int j = 0; j < EDGE_RARITY_N; j++) {
                    std::uint32_t seed = std::time(0);
                    MyGraph_t G;
                    _generate_graph(G, vert_counts[i], edge_rarities[j], seed);
                    std::vector<std::string> labels = {
                        std::to_string(seed), std::to_string(vert_counts[i]),
                        std::to_string(boost::num_edges(G)),
                        std::to_string(edge_rarities[j])};
                    _run(G, labels, file_prefix);
                    sleep(rest_times[i][j]);
                }
            }
        }
    }
    std::string data_dir = "datasets/";
    {
        MyGraph_t G;
        std::size_t vert_count = 4039;
        std::string file_prefix = output_dir + "facebook_";
        add_csv_header({}, file_prefix);
        _load_graph<UNDIRECTED>(G, vert_count, data_dir + "facebook_combined.txt", " ");
        std::vector<std::string> labels(0);
        for (int id = START_I; id <= END_I; id++) {
            _run(G, labels, file_prefix);
            sleep(5);
        }
    }
    {
        MyGraph_t G;
        std::size_t vert_count = 7115;
        std::string file_prefix = output_dir + "wiki-Vote_";
        add_csv_header({}, file_prefix);
        _load_graph<DIRECTED>(G, vert_count, data_dir + "wiki-Vote.txt", " ");
        std::vector<std::string> labels(0);
        for (int id = START_I; id <= END_I; id++) {
            _run(G, labels, file_prefix);
            sleep(10);
        }
    }
    {
        MyGraph_t G;
        std::size_t vert_count = 22470;
        std::string file_prefix = output_dir + "facebook_large_";
        add_csv_header({}, file_prefix);
        _load_graph<UNDIRECTED>(G, vert_count,
                                data_dir + "musae_facebook_edges.csv", ",");
        std::vector<std::string> labels(0);
        for (int id = START_I; id <= END_I; id++) {
            _run(G, labels, file_prefix);
            sleep(15);
        }
    }
    return EXIT_SUCCESS;
}
