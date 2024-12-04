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

int main()
{
    MyGraph_t G;

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
            if (std::rand() % (boost::out_degree(*i, G) + 1) == 0) {
                boost::add_edge(*i, j, G);
                // std::cout << G[*i].name << ", " << G[j].name << "\n";
            }
        }
    }

    std::size_t start_idx = std::rand() % boost::num_vertices(G);
    boost::vec_adj_list_vertex_id_map<VertData, VertIdx_t> vert_map =
        boost::get(boost::vertex_index, G);

    typedef boost::iterator_property_map<
        std::vector<std::size_t>::iterator,
        boost::property_map<MyGraph_t, boost::vertex_index_t>::const_type>
        dtime_pm_type;
    std::vector<std::size_t> dtime(boost::num_vertices(G));
    dtime_pm_type dtime_pm(dtime.begin(), vert_map);

    std::size_t time = 0;
    bfs_time_visitor<dtime_pm_type> vis1(dtime_pm, time);
    Timer timer;
    boost::breadth_first_search(G, vert_map[start_idx], boost::visitor(vis1));
    double delta1 = timer.elapsed();
    
    std::vector<std::size_t> dtime2(boost::num_vertices(G));
    dtime_pm_type dtime_pm2(dtime2.begin(), vert_map);

    bfs_time_visitor<dtime_pm_type> vis2(dtime_pm2, time);
    timer.reset();
    basic::breadth_first_search(G, vert_map[start_idx], vis2);
    double delta2 = timer.elapsed();

    bfs_time_visitor<dtime_pm_type> vis3(dtime_pm2, time);
    timer.reset();
    parallel::breadth_first_search(G, vert_map[start_idx], vis3);
    double delta3 = timer.elapsed();

    std::cout << vert_map[start_idx] << "\n";
    std::cout << delta1 << " vs " << delta2 << " vs " << delta3 << "\n";


    // for (std::size_t i = 0; i < dtime.size(); i++) {
    //     std::cout << dtime[i] << ", ";
    // }
    // std::cout << "\n";

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