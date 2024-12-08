#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <chrono>

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

template <typename T> std::map<T, std::size_t> get_freq_map(std::vector<T> v)
{
    std::map<T, std::size_t> freqs;
    for (T &e : v) {
        freqs[e]++;
    }
    return freqs;
}

std::string join_str(std::vector<std::string> &strs, std::string delim)
{
    std::string res = strs[0];
    for (auto str_i = strs.begin() + 1; str_i != strs.end(); str_i++) {
        res += delim + *str_i;
    }
    return res;
}