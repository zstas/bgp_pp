#ifndef EVLOOP_HPP
#define EVLOOP_HPP

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "table.hpp"

struct GlobalConf;
struct bgp_fsm;

struct EVLoop {
    // asio
    boost::asio::io_context &io;
    boost::asio::ip::tcp::acceptor accpt;
    boost::asio::ip::tcp::socket sock;

    GlobalConf &conf;
    bgp_table_v4 table;
    std::map<address_v4,std::shared_ptr<bgp_fsm>> neighbours;

    EVLoop( boost::asio::io_context &i, GlobalConf &c );

    void on_accept( boost::system::error_code ec );
    void on_vpp_accept( boost::system::error_code ec );
};

#endif