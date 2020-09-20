#ifndef MAIN_HPP
#define MAIN_HPP

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

using io_context = boost::asio::io_context;
using acceptor = boost::asio::ip::tcp::acceptor;
using endpoint = boost::asio::ip::tcp::endpoint;
using socket_tcp = boost::asio::ip::tcp::socket;
using error_code = boost::system::error_code;

#include "table.hpp"

struct GlobalConf;
struct bgp_fsm;

struct main_loop {
    // asio
    io_context io;
    acceptor accpt;
    socket_tcp sock;

    GlobalConf &conf;
    bgp_table_v4 table;
    std::map<address_v4,std::shared_ptr<bgp_fsm>> neighbours;

    main_loop( GlobalConf &c );

    void run(); 
    void on_accept( error_code ec );
    void on_vpp_accept( error_code ec );
};

#endif