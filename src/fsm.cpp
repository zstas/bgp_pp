#include <memory>
#include <vector>
#include <tuple>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "fsm.hpp"
#include "config.hpp"
#include "packet.hpp"
#include "log.hpp"
#include "string_utils.hpp"

extern Logger logger;

bgp_fsm::bgp_fsm( io_context &io,  GlobalConf &g, bgp_table_v4 &t, bgp_neighbour_v4 &c ):
    state( FSM_STATE::IDLE ),
    gconf( g ),
    conf( c ),
    table( t ),
    ConnectRetryTimer( io ),
    HoldTimer( io ),
    KeepaliveTimer( io )
{
    HoldTime = gconf.hold_time;
    if( conf.hold_time.has_value() ) {
        HoldTime = *conf.hold_time;
    }
}

void bgp_fsm::place_connection( socket_tcp s ) {
    sock.emplace( std::move( s ) );
    auto const &endpoint = sock->remote_endpoint();
    logger.logInfo() << LOGS::FSM << "Incoming connection: " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
    do_read();
    tx_open();
}

void bgp_fsm::start_keepalive_timer() {
    KeepaliveTimer.expires_from_now( std::chrono::seconds( KeepaliveTime ) );
    KeepaliveTimer.async_wait( std::bind( &bgp_fsm::on_keepalive_timer, shared_from_this(), std::placeholders::_1 ) );
}

void bgp_fsm::on_keepalive_timer( error_code ec ) {
    logger.logInfo() << LOGS::FSM << "Periodic KEEPALIVE" << std::endl;
    if( ec ) {
        logger.logInfo() << LOGS::FSM << "Lost connection" << std::endl;
        // todo change state
        start_keepalive_timer();
        return;
    }
    tx_keepalive();
    start_keepalive_timer();
}

void bgp_fsm::rx_open( bgp_packet &pkt ) {
    auto open = pkt.get_open();

    logger.logInfo() << LOGS::FSM << "Incoming OPEN packet from: " << sock->remote_endpoint().address().to_string() << std::endl;
    logger.logInfo() << LOGS::PACKET << open << std::endl;

    if( open->my_as.native() != conf.remote_as ) {
        logger.logError() << LOGS::FSM << "Incorrect AS: " << open->my_as.native() << ", we expected: " << conf.remote_as << std::endl;
        sock->close();
        return;
    }

    HoldTime = std::min( open->hold_time.native(), HoldTime );
    KeepaliveTime = HoldTime / 3;
    logger.logInfo() << LOGS::FSM << "Negotiated timers - hold_time: " << HoldTime << " keepalive_time: " << KeepaliveTime << std::endl;

    tx_keepalive();
    state = FSM_STATE::OPENCONFIRM;
}

void bgp_fsm::tx_open() {
    auto len = sizeof( bgp_header ) + sizeof( bgp_open );
    auto pkt_buf = std::make_shared<std::vector<uint8_t>>();
    pkt_buf->resize( len );
    bgp_packet pkt { pkt_buf->data(), pkt_buf->size() };

    // header
    auto header = pkt.get_header();
    header->type = bgp_type::OPEN;
    header->length = len;
    std::fill( header->marker.begin(), header->marker.end(), 0xFF );

    // open body
    auto open = pkt.get_open();
    open->version = 4;
    open->bgp_id = gconf.bgp_router_id.to_uint();
    open->my_as = gconf.my_as;
    if( conf.hold_time.has_value() ) {
        open->hold_time = *conf.hold_time;
    } else {
        open->hold_time = gconf.hold_time;
    }
    open->len = 0;

    // send this msg
    sock->async_send( boost::asio::buffer( *pkt_buf ), std::bind( &bgp_fsm::on_send, shared_from_this(), pkt_buf, std::placeholders::_1, std::placeholders::_2 ) );
    state = FSM_STATE::OPENSENT;
}

void bgp_fsm::on_send( std::shared_ptr<std::vector<uint8_t>> pkt, error_code ec, std::size_t length ) {
    if( ec ) {
        logger.logError() << LOGS::FSM << "Error on sending packet: " << ec.message() << std::endl;
        return;
    }
    logger.logInfo() << LOGS::FSM << "Successfully sent a message with size: " << length << std::endl;
}

void bgp_fsm::tx_keepalive() {
    logger.logInfo() << LOGS::FSM << "Sending KEEPALIVE to peer: " << sock->remote_endpoint().address().to_string() << std::endl;
    auto len = sizeof( bgp_header );
    auto pkt_buf = std::make_shared<std::vector<uint8_t>>();
    pkt_buf->resize( len );
    bgp_packet pkt { pkt_buf->data(), pkt_buf->size() };

    // header
    auto header = pkt.get_header();
    header->type = bgp_type::KEEPALIVE;
    header->length = len;
    std::fill( header->marker.begin(), header->marker.end(), 0xFF );

    // send this msg
    sock->async_send( boost::asio::buffer( *pkt_buf ), std::bind( &bgp_fsm::on_send, shared_from_this(), pkt_buf, std::placeholders::_1, std::placeholders::_2 ) );
}

void bgp_fsm::rx_keepalive( bgp_packet &pkt ) {
    if( state == FSM_STATE::OPENCONFIRM || state == FSM_STATE::OPENSENT ) {
        logger.logError() << LOGS::FSM << "BGP goes to ESTABLISHED state with peer: " << sock->remote_endpoint().address().to_string() << std::endl;
        state = FSM_STATE::ESTABLISHED;
        start_keepalive_timer();
    } else if( state != FSM_STATE::ESTABLISHED ) {
        logger.logError() << LOGS::FSM << "Received a KEEPALIVE in incorrect state, closing connection" << std::endl;
        sock->close();
    }
    logger.logInfo() << LOGS::FSM << "Received a KEEPALIVE message" << std::endl;
}

void bgp_fsm::rx_update( bgp_packet &pkt ) {
    auto [ withdrawn_routes, path_attrs, routes ] = pkt.process_update();
    logger.logInfo() << LOGS::FSM << "Received UPDATE message with withdrawn routes " << withdrawn_routes.size()
    << ", paths: " << path_attrs.size() << " and routes: " << routes.size() << std::endl;

    for( auto &wroute: withdrawn_routes ) {
        logger.logInfo() << LOGS::FSM << "Received withdrawn route: " << wroute << std::endl;
        table.del_path( wroute, shared_from_this() );
    }

    for( auto &route: routes ) {
        logger.logInfo() << LOGS::FSM << "Received route: " << route << std::endl;
        table.add_path( route, path_attrs, shared_from_this() );
    }

    logger.logInfo() << LOGS::FSM << "After update we have BGP table: " << std::endl;
    for( auto const &[ k, v ]: table.table ) {
        logger.logInfo() << LOGS::FSM << "Route: " << k.to_string() << std::endl;
        for( auto path: *v.attrs ) {
            logger.logInfo() << LOGS::FSM << "Path: " << path << std::endl;
        }
    }
}

void bgp_fsm::on_receive( error_code ec, std::size_t length ) {
    if( ec ) {
        logger.logError() << LOGS::FSM << "Error on receiving data: " << ec.message() << std::endl;
        return;
    }

    logger.logInfo() << LOGS::FSM << "Received message of size: " << length << std::endl;
    bgp_packet pkt { buffer.begin(), length };
    auto bgp_header = pkt.get_header();
    if( std::any_of( bgp_header->marker.begin(), bgp_header->marker.end(), []( uint8_t el ) { return el != 0xFF; } ) ) {
        logger.logInfo() << LOGS::FSM << "Wrong BGP marker in header!" << std::endl;
        return;
    }
    switch( bgp_header->type ) {
    case bgp_type::OPEN:
        rx_open( pkt );
        break;
    case bgp_type::KEEPALIVE:
        rx_keepalive( pkt );
        break;
    case bgp_type::UPDATE:
        rx_update( pkt );
        break;
    case bgp_type::NOTIFICATION:
        logger.logInfo() << LOGS::FSM << "NOTIFICATION message" << std::endl;
        break;
    case bgp_type::ROUTE_REFRESH:
        logger.logInfo() << LOGS::FSM << "ROUTE_REFRESH message" << std::endl;
        break;
    }
    do_read();
}

void bgp_fsm::do_read() {
    sock->async_receive( boost::asio::buffer( buffer ), std::bind( &bgp_fsm::on_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2 ) );
}
