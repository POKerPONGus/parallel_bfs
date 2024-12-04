#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <chrono>

enum VertColor { WHITE, GRAY, BLACK };

struct VertData {
    int idx;
    std::string name;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              VertData, boost::no_property>
    MyGraph_t;
typedef boost::graph_traits<MyGraph_t>::vertex_descriptor VertIdx_t;
typedef boost::graph_traits<MyGraph_t>::edge_descriptor EdgeIdx_t;

class Timer
{
private:
	// Type aliases to make accessing nested type easier
	using Clock = std::chrono::steady_clock;
	using Second = std::chrono::duration<double, std::ratio<1> >;

	std::chrono::time_point<Clock> m_beg { Clock::now() };

public:
	void reset()
	{
		m_beg = Clock::now();
	}

	double elapsed() const
	{
		return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
	}
};