#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <tuple>
#include <vector>

struct path_attr_t;
struct bgp_fsm;
enum class ORIGIN : uint8_t;
struct GlobalConf;

namespace std {
template<>
struct less<prefix_v4> {
    bool operator() (const prefix_v4& lhs, const prefix_v4& rhs) const
    {
        return ( lhs.address() < rhs.address() ) || ( lhs.prefix_length() < rhs.prefix_length() );
    }
};
}

struct bgp_path {
    std::shared_ptr<std::vector<path_attr_t>> attrs;
    std::chrono::system_clock::time_point time;
    std::shared_ptr<bgp_fsm> source;
    bool isValid;
    bool isBest;

    bgp_path( std::shared_ptr<std::vector<path_attr_t>> a, std::shared_ptr<bgp_fsm> s );

    uint32_t get_local_pref() const;
    uint32_t get_med() const;
    ORIGIN get_origin() const;
    std::vector<uint32_t> get_as_path() const;
};

struct bgp_table_v4 {
    bgp_table_v4( GlobalConf &c );
    GlobalConf &conf;
    std::multimap<prefix_v4,bgp_path> table;
    void add_path( prefix_v4 prefix, std::vector<path_attr_t> attr, std::shared_ptr<bgp_fsm> nei );
    void del_path( prefix_v4 prefix, std::shared_ptr<bgp_fsm> nei );
    void best_path_selection();
};

#endif