#ifndef EVLOOP_HPP
#define EVLOOP_HPP

#include <set>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "table.hpp"
#include "fsm.hpp"

struct GlobalConf;
struct bgp_fsm;

class EVLoop : public std::enable_shared_from_this<EVLoop> {
public:
    EVLoop( boost::asio::io_context &i, GlobalConf &c );
    void start();
    
    std::map<address_v4,std::shared_ptr<bgp_fsm>> neighbours;
    bgp_table_v4 table;
    void schedule_updates( std::set<nlri> &v );
private:
    void on_accept( const boost::system::error_code &ec );
    void on_send_updates( const boost::system::error_code &ec );

    GlobalConf &conf;
    // asio
    boost::asio::io_context &io;
    boost::asio::ip::tcp::acceptor accpt;
    boost::asio::ip::tcp::socket sock;
    boost::asio::steady_timer send_updates;

    std::list<nlri> planning_updates;
};

#endif