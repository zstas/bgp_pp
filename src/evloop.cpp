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
    table( c ),
    send_updates( io )
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

void EVLoop::schedule_updates( std::list<nlri> &v ) {
    for( auto const &n: v ) {
        planning_updates.push_back( n );
    }
    send_updates.expires_from_now( std::chrono::seconds( 1 ) );
    send_updates.async_wait( std::bind( &EVLoop::on_send_updates, shared_from_this(), std::placeholders::_1 ) );
}

void EVLoop::on_send_updates( const boost::system::error_code &ec ) {
    if( ec ) {
        logger.logError() << LOGS::EVENT_LOOP << "On timer for sending updates: " << ec.message() << std::endl;
    }
    std::vector<nlri> withdrawn_update;
    std::map<std::shared_ptr<std::vector<path_attr_t>>,std::vector<nlri>> pending_update;
    for( auto const &n: planning_updates ) {
        auto it = table.table.end();
        if( it = table.table.find( n ); it == table.table.end() ) {
            withdrawn_update.push_back( n );
            continue;
        }
        if( auto updIt = pending_update.find( it->second.attrs ); updIt != pending_update.end() ) {
            updIt->second.push_back( n );
        } else {
            std::vector<nlri> new_vec { n };
            pending_update.emplace( it->second.attrs, new_vec );
        }
    }

    for( auto const &[ add, nei ]: neighbours ) {
        // send updates only for eBGP neighbours
        if( nei->conf.remote_as == conf.my_as ) {
            continue;
        }

        if( nei->state != FSM_STATE::ESTABLISHED ) {
            continue;
        }

        auto cur_withdrawn = withdrawn_update;
        for( auto const &[ path, n_vec ]: pending_update ) {
            nei->tx_update( n_vec, path, cur_withdrawn );
            cur_withdrawn.clear();
        }
    }
}