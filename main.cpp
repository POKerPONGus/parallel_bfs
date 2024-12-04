#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <queue>

#include "main.hpp"
#include "basic_bfs.hpp"
#include "parallel_bfs.hpp"

template <typename TimeMap>
class bfs_time_visitor : public boost::default_bfs_visitor {
    typedef typename boost::property_traits<TimeMap>::value_type T;

  public:
    bfs_time_visitor(TimeMap tmap, T &t) : m_timemap(tmap), m_time(t) {}


    template <typename Vertex, typename Graph>
    void initialize_vertex(Vertex u, const Graph &g) const
    {
        put(m_timemap, u, -1);
    }
    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph &g) const
    {
        put(m_timemap, u, m_time++);
    }
    TimeMap m_timemap;
    T &m_time;
};

template <typename DistMap>
class bfs_dist_visitor : public boost::default_bfs_visitor {
    typedef typename boost::property_traits<DistMap>::value_type T;

  public:
    bfs_dist_visitor(DistMap dist_map) : _dist_map(dist_map) {}

    template <typename Vertex, typename Graph>
    void initialize_vertex(Vertex u, const Graph &g) const
    {
        put(_dist_map, u, 0);
    }
    template <typename Edge, typename Graph>
    void tree_edge(Edge e, const Graph &g) const
    {
        put(_dist_map, target(e, g), get(_dist_map, source(e, g)) + 1);
    }
    DistMap _dist_map;
};

#define VERT_CNT 10000
#define EDGE_RARITY 100

static void _generate_graph(MyGraph_t &G) {
    for (int i = 0; i < VERT_CNT; i++) {
        VertIdx_t v = boost::add_vertex(G);
        G[v].idx = i;
        G[v].name = "vertex no." + std::to_string(i);
    }
    std::srand(std::time(0));
    std::random_device rd;
    std::mt19937 generator(rd());

    typedef boost::range_detail::integer_iterator<std::size_t> SizeIter_t;
    std::pair<SizeIter_t, SizeIter_t> vert_pair = boost::vertices(G);
    for (SizeIter_t i = vert_pair.first; i != vert_pair.second; i++) {

        std::vector<VertIdx_t> verts_vec(vert_pair.first, vert_pair.second);
        std::shuffle(verts_vec.begin(), verts_vec.end(), generator);

        for (VertIdx_t j : verts_vec) {
            if (std::rand() % (EDGE_RARITY*boost::out_degree(*i, G) + 1) == 0) {
                boost::add_edge(*i, j, G);
                // std::cout << G[*i].name << ", " << G[j].name << "\n";
            }
        }
    }
}

void _time_based_test(MyGraph_t &G, VertIdx_t start_idx) {
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);
        
    typedef boost::iterator_property_map<
        std::vector<std::size_t>::iterator,
        boost::property_map<MyGraph_t, boost::vertex_index_t>::const_type>
        dtime_pm_type;
    std::size_t time;

    time = 0;
    std::vector<std::size_t> dtime1(boost::num_vertices(G));
    dtime_pm_type dtime_pm1(dtime1.begin(), vert_map);
    bfs_time_visitor<dtime_pm_type> vis1(dtime_pm1, time);
    Timer timer;
    boost::breadth_first_search(G, vert_map[start_idx], boost::visitor(vis1));
    double delta1 = timer.elapsed();

    time = 0;
    std::vector<std::size_t> dtime2(boost::num_vertices(G));
    dtime_pm_type dtime_pm2(dtime2.begin(), vert_map);
    bfs_time_visitor<dtime_pm_type> vis2(dtime_pm2, time);
    timer.reset();
    basic::breadth_first_search(G, vert_map[start_idx], vis2);
    double delta2 = timer.elapsed();

    time = 0;
    std::vector<std::size_t> dtime3(boost::num_vertices(G));
    dtime_pm_type dtime_pm3(dtime3.begin(), vert_map);
    bfs_time_visitor<dtime_pm_type> vis3(dtime_pm3, time);
    timer.reset();
    parallel::breadth_first_search(G, vert_map[start_idx], vis3);
    double delta3 = timer.elapsed();

    std::cout << vert_map[start_idx] << "\n";
    std::cout << delta1 << " vs " << delta2 << " vs " << delta3 << "\n";
    std::cout << time << "\n";
    std::cout << "dtime 1 == 2 : " << ((dtime1 == dtime2)? "true" : "false") << "\n";
    std::cout << "dtime 1 == 3: " << ((dtime1 == dtime3)? "true" : "false") << "\n";
    std::cout << "dtime 2 == 3: " << ((dtime2 == dtime3)? "true" : "false") << "\n";

    for (auto time : dtime1) {
        std::cout << time << ", ";
    }
    std::cout << "\n";
    for (auto time : dtime2) {
        std::cout << time << ", ";
    }
    std::cout << "\n";    
    for (auto time : dtime3) {
        std::cout << time << ", ";
    }
    std::cout << "\n";
}

template <typename T>
std::map<T, std::size_t> _get_freq_map(std::vector<T> v) {
    std::map<T, std::size_t> freqs;
    for (T &e : v){
        freqs[e]++;
    }
    return freqs;
}

void _dist_based_test(MyGraph_t &G, VertIdx_t start_idx) {
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);
        
    typedef boost::iterator_property_map<
        std::vector<std::size_t>::iterator,
        boost::property_map<MyGraph_t, boost::vertex_index_t>::const_type>
        DistPM_t;

    std::vector<std::size_t> dist1(boost::num_vertices(G));
    DistPM_t dist_pm1(dist1.begin(), vert_map);
    bfs_dist_visitor<DistPM_t> vis1(dist_pm1);
    Timer timer;
    boost::breadth_first_search(G, vert_map[start_idx], boost::visitor(vis1));
    double delta1 = timer.elapsed();

    std::vector<std::size_t> dist2(boost::num_vertices(G));
    DistPM_t dist_pm2(dist2.begin(), vert_map);
    bfs_dist_visitor<DistPM_t> vis2(dist_pm2);
    timer.reset();
    basic::breadth_first_search(G, vert_map[start_idx], vis2);
    double delta2 = timer.elapsed();

    std::vector<std::size_t> dist3(boost::num_vertices(G));
    DistPM_t dist_pm3(dist3.begin(), vert_map);
    bfs_dist_visitor<DistPM_t> vis3(dist_pm3);
    timer.reset();
    parallel::breadth_first_search(G, vert_map[start_idx], vis3);
    double delta3 = timer.elapsed();

    std::cout << vert_map[start_idx] << "\n";
    std::cout << delta1 << " vs " << delta2 << " vs " << delta3 << "\n";
    std::cout << time << "\n";
    std::cout << "dist 1 == 2 : " << ((dist1 == dist2)? "true" : "false") << "\n";
    std::cout << "dist 1 == 3: " << ((dist1 == dist3)? "true" : "false") << "\n";
    std::cout << "dist 2 == 3: " << ((dist2 == dist3)? "true" : "false") << "\n";

    std::map<std::size_t, std::size_t> dist1_freqs = _get_freq_map(dist1);
    std::map<std::size_t, std::size_t> dist2_freqs = _get_freq_map(dist2);
    std::map<std::size_t, std::size_t> dist3_freqs = _get_freq_map(dist3);
    for (auto kvp : dist1_freqs) {
        std::cout << kvp.first << ": " << kvp.second << ", ";
    }
    std::cout << "\n";
    for (auto kvp : dist2_freqs) {
        std::cout << kvp.first << ": " << kvp.second << ", ";
    }
    std::cout << "\n";    
    for (auto kvp : dist3_freqs) {
        std::cout << kvp.first << ": " << kvp.second << ", ";
    }
    std::cout << "\n";
}

int main()
{
    MyGraph_t G;
    _generate_graph(G);

    std::size_t start_idx = std::rand() % boost::num_vertices(G);
    _dist_based_test(G, start_idx);
    // // Use std::sort to order the vertices by their discover time
    // std::vector<std::size_t> discover_order(boost::num_vertices(G));
    // boost::integer_range<int> range(0, boost::num_vertices(G));
    // std::copy(range.begin(), range.end(), discover_order.begin());
    // std::sort(
    //     discover_order.begin(), discover_order.end(),
    //     boost::indirect_cmp<dtime_pm_type, std::less<std::size_t>>(dtime_pm));

    // std::cout << "\nCheck results\n\n";
    // std::cout << "Start " << vert_map[start_idx] << "\n";
    // for (std::size_t i = 0; i < boost::num_vertices(G); i++) {
    //     std::cout << vert_map[discover_order[i]] << ", ";
    // }
    // std::cout << "\n";

    // boost::dynamic_properties attr;
    // attr.property("node_id", vert_map);
    // attr.property("concentrate",
    //               boost::make_constant_property<MyGraph_t *>(true));

    // std::ofstream file("graph.dot");
    // boost::write_graphviz_dp(file, G, attr);
    // file.close();
    return EXIT_SUCCESS;
}