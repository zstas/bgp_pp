#include <boost/asio/ip/address_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "evloop.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "nlri.hpp"
#include "config.hpp"
#include "fsm.hpp"
#include "packet.hpp"

extern Logger logger;

EVLoop::EVLoop( boost::asio::io_context &i, GlobalConf &c ):
    io( i ),
    conf( c ),
    accpt( i, endpoint( boost::asio::ip::tcp::v4(), c.listen_on_port ) ),
    sock( i ),
    table( i, c )
{
    for( auto &nei: c.neighbours ) {
        neighbours.emplace( nei.address, std::make_shared<bgp_fsm>( io, c, table, nei ) );
    }
}

void EVLoop::start() {
    accpt.async_accept( sock, std::bind( &EVLoop::on_accept, shared_from_this(), std::placeholders::_1 ) );
}

void EVLoop::on_accept( const boost::system::error_code &ec ) {
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
    accpt.async_accept( sock, std::bind( &EVLoop::on_accept, shared_from_this(), std::placeholders::_1 ) );
}
