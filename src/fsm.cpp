#include <memory>
#include <vector>
#include <tuple>
#include <set>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "fsm.hpp"
#include "nlri.hpp"
#include "config.hpp"
#include "packet.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "evloop.hpp"

extern Logger logger;
extern std::shared_ptr<EVLoop> runtime;

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
    if( sock.has_value() ) {
        sock->close();
        sock.reset();

        KeepaliveTimer.cancel();
    }
    sock.emplace( std::move( s ) );
    auto const &endpoint = sock->remote_endpoint();
    logger.logInfo() << LOGS::FSM << "Incoming connection: " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
    do_read();
    std::set<bgp_cap_t> capabilites;
    bgp_cap_t rr;
    rr.make_route_refresh();
    capabilites.emplace( rr );
    rr.make_4byte_asn( gconf.my_as );
    capabilites.emplace( rr );
    rr.make_mp_bgp( BGP_AFI::IPv4, BGP_SAFI::UNICAST );
    capabilites.emplace( rr );
    rr.make_fqdn( "myhost", "mydomain" );
    capabilites.emplace( rr );
    tx_open( capabilites );
}

void bgp_fsm::start_keepalive_timer() {
    KeepaliveTimer.expires_from_now( std::chrono::seconds( KeepaliveTime ) );
    KeepaliveTimer.async_wait( std::bind( &bgp_fsm::on_keepalive_timer, shared_from_this(), std::placeholders::_1 ) );
}

void bgp_fsm::on_keepalive_timer( error_code ec ) {
    if( ec ) {
        logger.logInfo() << LOGS::FSM << "Keepaliva timer: " << ec.message() << std::endl;
        // todo change state
        return;
    }
    logger.logInfo() << LOGS::FSM << "Periodic KEEPALIVE" << std::endl;
    tx_keepalive();
    start_keepalive_timer();
}

void bgp_fsm::rx_open( bgp_packet &pkt ) {
    auto open = pkt.get_open();

    logger.logInfo() << LOGS::FSM << "Incoming OPEN packet from: " << sock->remote_endpoint().address().to_string() << std::endl;
    logger.logInfo() << LOGS::PACKET << open << std::endl;

    caps = open->parse_capabilites();
    for( auto const &cap: caps ) {
        logger.logInfo() << LOGS::PACKET << cap << std::endl;
    }

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

void bgp_fsm::tx_open( const std::set<bgp_cap_t> &caps ) {
    std::vector<uint8_t> caps_bytes;
    for( auto const &c: caps ) {
        auto b = c.toBytes();
        caps_bytes.insert( caps_bytes.end(), b.begin(), b.end() );
    }
    
    auto len = sizeof( bgp_header ) + sizeof( bgp_open );
    auto pkt_buf = std::make_shared<std::vector<uint8_t>>();
    pkt_buf->reserve( len + caps_bytes.size() );
    pkt_buf->resize( len );
    bgp_packet pkt { pkt_buf->data(), pkt_buf->size() };

    // header
    auto header = pkt.get_header();
    header->type = bgp_type::OPEN;
    header->length = len + caps_bytes.size();
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
    open->len = caps_bytes.size();

    pkt_buf->insert( pkt_buf->end(), caps_bytes.begin(), caps_bytes.end() );

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
        send_all_prefixes();
    } else if( state != FSM_STATE::ESTABLISHED ) {
        logger.logError() << LOGS::FSM << "Received a KEEPALIVE in incorrect state, closing connection" << std::endl;
        sock->close();
    }
    logger.logInfo() << LOGS::FSM << "Received a KEEPALIVE message" << std::endl;
}

void bgp_fsm::rx_update( bgp_packet &pkt ) {
    std::set<NLRI> schedule;
    auto cap_it = std::find_if( caps.begin(), caps.end(), []( const bgp_cap_t &val ) -> bool { return val.code == BGP_CAP_CODE::FOUR_OCT_AS; } );
    auto four_byte_asn = ( cap_it != caps.end() );
    auto [ withdrawn_routes, path_attrs, routes ] = pkt.process_update( four_byte_asn );
    logger.logInfo() << LOGS::FSM << "Received UPDATE message with withdrawn routes " << withdrawn_routes.size()
    << ", paths: " << path_attrs.size() << " and routes: " << routes.size() << std::endl;

    for( auto const &a: path_attrs ) {
        if( a.type != PATH_ATTRIBUTE::AS_PATH )
            continue;
        auto ases = a.parse_as_path();
        auto it = std::find( ases.begin(), ases.end(), gconf.my_as );
        if( it != ases.end() ) {
            logger.logInfo() << LOGS::FSM << "Do not process this update because our AS found in AS_PATH attribute" << std::endl;
            return;
        }
    }

    for( auto &wroute: withdrawn_routes ) {
        schedule.emplace( wroute );
        logger.logInfo() << LOGS::FSM << "Received withdrawn route: " << wroute << std::endl;
        table.del_path( wroute, shared_from_this() );
    }

    for( auto &route: routes ) {
        schedule.emplace( route );
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

    table.best_path_selection();
}

void bgp_fsm::on_receive( error_code ec, std::size_t length ) {
    if( ec ) {
        logger.logError() << LOGS::FSM << "Error on receiving data: " << ec.message() << std::endl;
        return;
    }

    logger.logInfo() << LOGS::FSM << "Received message of size: " << length << std::endl;

    std::list<bgp_packet> pkts;
    auto pos = 0;
    while( pos < length ) {
        auto header = reinterpret_cast<bgp_header*>( buffer.data() + pos );
        auto len = header->length.native();
        logger.logInfo() << LOGS::FSM << "Next packet in stream with size: " << len << std::endl;
        if( ( pos + len ) > length ) {
            break;
        }
        pkts.emplace_back( buffer.data() + pos, len );
        pos += len;
    }

    for( auto &pkt: pkts ) {
        auto bgp_header = pkt.get_header();
        if( std::any_of( bgp_header->marker.begin(), bgp_header->marker.end(), []( uint8_t el ) { return el != 0xFF; } ) ) {
            logger.logError() << LOGS::FSM << "Wrong BGP marker in header!" << std::endl;
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
            rx_notification( pkt );
            break;
        case bgp_type::ROUTE_REFRESH:
            logger.logInfo() << LOGS::FSM << "ROUTE_REFRESH message" << std::endl;
            send_all_prefixes();
            break;
        }
    }
    do_read();
}

void bgp_fsm::do_read() {
    sock->async_receive( boost::asio::buffer( buffer ), std::bind( &bgp_fsm::on_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2 ) );
}

void bgp_fsm::tx_update( const std::vector<NLRI> &prefixes, std::shared_ptr<std::vector<path_attr_t>> path, const std::vector<NLRI> &withdrawn ) {
    logger.logInfo() << LOGS::FSM << "Sending UPDATE to peer: " << sock->remote_endpoint().address().to_string() << std::endl;

    auto new_path = *path;
    auto cap_it = std::find_if( caps.begin(), caps.end(), []( const bgp_cap_t &val ) -> bool { return val.code == BGP_CAP_CODE::FOUR_OCT_AS; } );
    auto four_byte_asn = ( cap_it != caps.end() );

    // For eBGP peer
    if( gconf.my_as != conf.remote_as ) {
        // remove local pref attribute
        new_path.erase(
            std::remove_if(
                new_path.begin(),
                new_path.end(),
                []( const path_attr_t &a ) -> bool { return a.type == PATH_ATTRIBUTE::LOCAL_PREF; }
            ),
            new_path.end()
        );

        // set next hop to output interface
        auto nexthopIt = std::find_if(
            new_path.begin(),
            new_path.end(),
            []( const path_attr_t &attr ) -> bool {
                return attr.type == PATH_ATTRIBUTE::NEXT_HOP;
            }
        ); 
        if( nexthopIt != new_path.end() ) {
            nexthopIt->make_nexthop( sock->local_endpoint().address() );
        }

        // put our as in as_path attribute
        auto aspathIt = std::find_if(
            new_path.begin(),
            new_path.end(),
            []( const path_attr_t &attr ) -> bool {
                return attr.type == PATH_ATTRIBUTE::AS_PATH;
            }
        ); 
        if( aspathIt != new_path.end() ) {
            auto new_as_path = aspathIt->parse_as_path();
            new_as_path.push_back( gconf.my_as );
            aspathIt->make_as_path( new_as_path );
        } else {
            path_attr_t as_path;
            as_path.four_byte_asn = four_byte_asn;
            as_path.make_as_path( { gconf.my_as } );
            new_path.push_back( as_path );
        }
    }

    // making withdrawn buf
    std::vector<uint8_t> withdrawn_body;
    withdrawn_body.reserve( 1000 );

    for( auto const &w: withdrawn ) {
        auto wdata = w.serialize();
        withdrawn_body.insert( withdrawn_body.end(), wdata.begin(), wdata.end() );
    }

    {
        uint16_t len = bswap( static_cast<uint16_t>( withdrawn_body.size() ) );
        std::array<uint8_t,2> temp;
        std::memcpy( temp.data(), &len, 2 );
        withdrawn_body.insert( withdrawn_body.begin(), temp.begin(), temp.end() );
    }

    // making path buf
    std::vector<uint8_t> path_body;
    path_body.reserve( 1000 );

    for( auto const &p: new_path ) {
        logger.logInfo() << LOGS::FSM << "Sending path: " << p << std::endl;
        auto bytes = p.to_bytes();
        path_body.insert( path_body.end(), bytes.begin(), bytes.end() );
    }

    {
        uint16_t len = bswap( static_cast<uint16_t>( path_body.size() ) );
        std::array<uint8_t,2> temp;
        std::memcpy( temp.data(), &len, 2 );
        path_body.insert( path_body.begin(), temp.begin(), temp.end() );
    }

    // making nlri buf
    std::vector<uint8_t> nlri_body;
    nlri_body.reserve( 1000 );

    for( auto const &p: prefixes ) {
        logger.logInfo() << LOGS::FSM << "Sending prefix: " << p.to_string() << std::endl;
        auto pdata = p.serialize();
        nlri_body.insert( nlri_body.end(), pdata.begin(), pdata.end() );
    }

    std::vector<uint8_t> body { withdrawn_body.begin(), withdrawn_body.end() };
    body.insert( body.end(), path_body.begin(), path_body.end() );
    body.insert( body.end(), nlri_body.begin(), nlri_body.end() );

    // making packet itself

    auto pkt_buf = std::make_shared<std::vector<uint8_t>>();
    auto len = sizeof( bgp_header ) + body.size();
    pkt_buf->resize( len );
    bgp_packet pkt { pkt_buf->data(), pkt_buf->size() };

    // header
    auto header = pkt.get_header();
    header->type = bgp_type::UPDATE;
    header->length = len;
    std::fill( header->marker.begin(), header->marker.end(), 0xFF );

    std::memcpy( pkt.get_body(), body.data(), body.size() );

    // send this msg
    sock->async_send( boost::asio::buffer( *pkt_buf ), std::bind( &bgp_fsm::on_send, shared_from_this(), pkt_buf, std::placeholders::_1, std::placeholders::_2 ) );
}

void bgp_fsm::send_all_prefixes() {
    std::set<NLRI> scheduled;
    bool ibgp = ( gconf.my_as == conf.remote_as );
    for( auto const &[ prefix, path ] : table.table ) {
        if( path.source && path.source->conf.remote_as == gconf.my_as ) {
            if( !ibgp ) {
                scheduled.emplace( prefix );
            }
        } else {
            scheduled.emplace( prefix );
        }
    }
}

void bgp_fsm::rx_notification( bgp_packet &pkt ) {
    logger.logInfo() << LOGS::FSM << "NOTIFICATION message" << std::endl;

    // clear all nlris from this peer
    table.purge_peer( shared_from_this() );

    auto notification = pkt.get_notification();
    logger.logInfo() << LOGS::FSM << notification << std::endl;
    if( sock.has_value() ) {
        sock->close();
        sock.reset();
    }
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, BGP_MSG_HDR_ERR err, const std::vector<uint8_t> &data ) {
    tx_notification( code, static_cast<uint8_t>( err ), data );
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, BGP_OPEN_ERR err, const std::vector<uint8_t> &data ) {
    tx_notification( code, static_cast<uint8_t>( err ), data );
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, BGP_UPDATE_ERR err, const std::vector<uint8_t> &data ) {
    tx_notification( code, static_cast<uint8_t>( err ), data );
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, BGP_FSM_ERR err, const std::vector<uint8_t> &data ) {
    tx_notification( code, static_cast<uint8_t>( err ), data );
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, BGP_CEASE_ERR err, const std::vector<uint8_t> &data ) {
    tx_notification( code, static_cast<uint8_t>( err ), data );
}

void bgp_fsm::tx_notification( BGP_ERR_CODE code, uint8_t subcode, const std::vector<uint8_t> &data ) {
    logger.logInfo() << LOGS::FSM << "Sending NOTIFICATION message" << std::endl;

    // clear all nlris from this peer
    table.purge_peer( shared_from_this() );

    auto pkt_buf = std::make_shared<std::vector<uint8_t>>();
    auto len = sizeof( bgp_header ) + sizeof( bgp_notification );
    pkt_buf->reserve( len + data.size() );
    pkt_buf->resize( len );
    bgp_packet pkt { pkt_buf->data(), pkt_buf->size() };

    // header
    auto header = pkt.get_header();
    header->type = bgp_type::NOTIFICATION;
    header->length = len + data.size();
    std::fill( header->marker.begin(), header->marker.end(), 0xFF );

    // notification
    auto notification = pkt.get_notification();
    notification->code = code;
    notification->subcode = subcode;

    pkt_buf->insert( pkt_buf->end(), data.begin(), data.end() );

    logger.logInfo() << LOGS::FSM << notification << std::endl;
    if( !sock.has_value() ) {
        logger.logInfo() << LOGS::FSM << "Cannot send NOTIFICATION because there are no active socket" << std::endl;
        return;
    }
    sock->async_send( boost::asio::buffer( *pkt_buf ), std::bind( &bgp_fsm::on_send, shared_from_this(), pkt_buf, std::placeholders::_1, std::placeholders::_2 ) );
}
