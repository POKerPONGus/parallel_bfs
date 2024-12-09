#pragma once

#include <boost/graph/adjacency_list.hpp>

template <typename TimeMap>
class BFSTimeVisitor : public boost::default_bfs_visitor {
  private:
    typedef typename boost::property_traits<TimeMap>::value_type T;
    TimeMap m_timemap;
    T m_time;

  public:
    BFSTimeVisitor() {}
    BFSTimeVisitor(TimeMap tmap, T t) : m_timemap(tmap), m_time(t) {}

    template <typename Vertex, typename Graph>
    void initialize_vertex(Vertex u, const Graph &g) const
    {
        put(m_timemap, u, -1);
    }
    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph &g)
    {
        put(m_timemap, u, m_time);
        m_time++;
    }
};

template <typename DistMap>
class BFSDistVisitor : public boost::default_bfs_visitor {
  private:
    typedef typename boost::property_traits<DistMap>::value_type T;
    DistMap _dist_map;

  public:
    BFSDistVisitor() {}
    BFSDistVisitor(DistMap dist_map) : _dist_map(dist_map) {}

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
};