#ifndef EVLOOP_HPP
#define EVLOOP_HPP

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "table.hpp"

struct GlobalConf;
struct bgp_fsm;

class EVLoop {
public:
    EVLoop( boost::asio::io_context &i, GlobalConf &c );
    
    std::map<address_v4,std::shared_ptr<bgp_fsm>> neighbours;
    bgp_table_v4 table;
private:
    void on_accept( boost::system::error_code ec );
    void on_vpp_accept( boost::system::error_code ec );

    GlobalConf &conf;
    // asio
    boost::asio::io_context &io;
    boost::asio::ip::tcp::acceptor accpt;
    boost::asio::ip::tcp::socket sock;
};

#endif