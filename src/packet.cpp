#include <vector>
#include <boost/asio/ip/network_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "packet.hpp"
#include "log.hpp"
#include "string_utils.hpp"

extern Logger logger;

path_attr_t::path_attr_t( path_attr_header *header ):
    optional( header->optional ),
    transitive( header->transitive ),
    partial( header->partial ),
    extended_length( header->extended_length ),
    type( header->type )
{
    auto len = header->extended_length == 1 ? header->ext_len.native() : header->len;
    auto body = reinterpret_cast<uint8_t*>( header ) + 2 + ( header->extended_length ? 2 : 1 );
    bytes = std::vector<uint8_t>( body, body + len );
}

uint32_t path_attr_t::get_u32() const {
    return bswap( *reinterpret_cast<const uint32_t*>( bytes.data() ) );
}

bgp_packet::bgp_packet( uint8_t *begin, std::size_t l ):
    data( begin ),
    length( l )
{}

bgp_header* bgp_packet::get_header() {
    return reinterpret_cast<bgp_header*>( data );
}

bgp_open* bgp_packet::get_open() {
    return reinterpret_cast<bgp_open*>( data + sizeof( bgp_header ) );
}

std::tuple<std::vector<nlri>,std::vector<path_attr_t>,std::vector<nlri>> bgp_packet::process_update() {
    std::vector<nlri> withdrawn_routes;
    auto header = get_header();
    auto update_data = data + sizeof( bgp_header );
    auto update_len = header->length.native() - sizeof( bgp_header );
    logger.logInfo() << LOGS::PACKET << "Size of UPDATE payload: " << update_len << std::endl;

    // parsing withdrawn routes
    auto len = bswap( *reinterpret_cast<uint16_t*>( update_data ) );
    logger.logInfo() << LOGS::PACKET << "Length of withdrawn routes: " << len << std::endl;
    uint16_t offset = sizeof( len );
    while( len > 0 ) {
        if( offset >= update_len ) {
            logger.logError() << LOGS::PACKET << "Error on parsing message" << std::endl;
            return { {}, {}, {} };
        }

        uint8_t nlri_len = *reinterpret_cast<uint8_t*>( update_data + offset );
        len -= sizeof( nlri_len );
        
        auto bytes = nlri_len / 8;
        if( nlri_len % 8 != 0 ) {
            bytes++;
        }

        if( bytes > len ) {
            logger.logError() << LOGS::PACKET << "Error on parsing message" << std::endl;
            return { {}, {}, {} };
        }
        len -= bytes;

        uint32_t address = 0;
        std::memcpy( &address, update_data + offset + 1, bytes );
        withdrawn_routes.emplace_back( address_v4 { bswap( address ) }, nlri_len );

        offset += sizeof( nlri_len ) + bytes;
    }

    // parsing bgp path attributes
    std::vector<path_attr_t> paths;
    len = bswap( *reinterpret_cast<uint16_t*>( update_data + offset ) );
    logger.logInfo() << LOGS::PACKET << "Length of path attributes: " << len << std::endl;
    offset += sizeof( len );
    while( len > 0 ) {
        auto path = reinterpret_cast<path_attr_header*>( update_data + offset );
        paths.emplace_back( path );
        auto attr_len = 3 + ( path->extended_length == 1 ? ( path->ext_len.native() + 1 ) : path->len );
        len -= attr_len;
        offset += attr_len;
    };

    // parsing NLRI
    len = update_len - offset;
    logger.logInfo() << LOGS::PACKET << "Length of NLRI: " << len << std::endl;
    std::vector<nlri> routes;
    while( len > 0 ) {
        if( offset >= update_len ) {
            logger.logError() << LOGS::PACKET << "Error on parsing message" << std::endl;
            return { {}, {}, {} };
        }

        uint8_t nlri_len = *reinterpret_cast<uint8_t*>( update_data + offset );
        len -= sizeof( nlri_len );
        
        auto bytes = nlri_len / 8;
        if( nlri_len % 8 != 0 ) {
            bytes++;
        }

        if( bytes > len ) {
            logger.logError() << LOGS::PACKET << "Error on parsing message" << std::endl;
            return { {}, {}, {} };
        }
        len -= bytes;

        uint32_t address = 0;
        std::memcpy( &address, update_data + offset + 1, bytes );
        routes.emplace_back( address_v4 { bswap( address ) }, nlri_len );
        
        offset += sizeof( nlri_len ) + bytes;
    }

    return { withdrawn_routes, paths, routes };
}

bool operator==( const path_attr_t &lhs, const path_attr_t &rhs ) {
    return  lhs.optional == rhs.optional &&
            lhs.transitive == rhs.transitive &&
            lhs.partial == rhs.partial &&
            lhs.extended_length == rhs.extended_length &&
            lhs.type == rhs.type &&
            lhs.bytes == rhs.bytes;
}