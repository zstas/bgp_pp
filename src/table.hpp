#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <tuple>
#include <vector>
#include <set>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

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

class bgp_table_v4 {
public:
    bgp_table_v4( boost::asio::io_context &i, GlobalConf &c );
    GlobalConf &conf;
    std::multimap<prefix_v4,bgp_path> table;
    void add_path( const prefix_v4 &prefix, std::vector<path_attr_t> attr, std::shared_ptr<bgp_fsm> peer );
    void del_path( const prefix_v4 &prefix, std::shared_ptr<bgp_fsm> peer );
    void purge_peer( std::shared_ptr<bgp_fsm> peer );
    void best_path_selection();
    void best_path_selection( const prefix_v4 &prefix );
private:
    void schedule_updates();
    void on_send_updates( const boost::system::error_code &ec );

    boost::asio::io_context &io;
    boost::asio::steady_timer send_updates;
    std::set<nlri> scheduled_updates;
};

#endif