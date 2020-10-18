#include <boost/asio/ip/network_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "evloop.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "config.hpp"
#include "fsm.hpp"
#include "packet.hpp"

extern Logger logger;

EVLoop::EVLoop( boost::asio::io_context &i, GlobalConf &c ):
    io( i ),
    conf( c ),
    accpt( io, endpoint( boost::asio::ip::tcp::v4(), c.listen_on_port ) ),
    sock( io ),
    table( c )
{
    for( auto &nei: c.neighbours ) {
        neighbours.emplace( nei.address, std::make_shared<bgp_fsm>( io, c, table, nei ) );
    }
    accpt.async_accept( sock, std::bind( &EVLoop::on_accept, this, std::placeholders::_1 ) );
}

void EVLoop::on_accept( boost::system::error_code &ec ) {
    if( ec ) {
        logger.logError() << LOGS::EVENT_LOOP << "Error on accepting new connection: " << ec.message() << std::endl;
    }
    auto const &remote_addr = sock.remote_endpoint().address().to_v4();
    auto const &nei_it = neighbours.find( remote_addr );
    if( nei_it == neighbours.end() ) {
        logger.logInfo() << LOGS::EVENT_LOOP << "Connection not from our peers, so dropping it." << std::endl;
        sock.close();
    } else {
        nei_it->second->place_connection( std::move( sock ) );
    }
    accpt.async_accept( sock, std::bind( &EVLoop::on_accept, this, std::placeholders::_1 ) );
}

void EVLoop::schedule_updates( std::vector<nlri> &v ) {
    for( auto const &n: v ) {
        planning_updates.push_back( n );
    }
    send_updates.expires_from_now( std::chrono::seconds( 1 ) );
    send_updates.async_wait( std::bind( &EVLoop::on_send_updates, this, std::placeholders::_1 ) );
}

void EVLoop::on_send_updates( boost::system::error_code &ec ) {
    std::map<std::vector<nlri>,path_attr_t> pending_update;
    for( auto const &n: planning_updates ) {

    }
    for( auto const &[ add, nei ]: neighbours ) {
        // send updates only for eBGP neighbours
        if( nei->conf.remote_as == conf.my_as ) {
            continue;
        }
        // nei->tx_update();
    }
}