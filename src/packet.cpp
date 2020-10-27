#include <vector>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "packet.hpp"
#include "log.hpp"
#include "string_utils.hpp"

extern Logger logger;

path_attr_t::path_attr_t( path_attr_header *header, bool f ):
    optional( header->optional ),
    transitive( header->transitive ),
    partial( header->partial ),
    extended_length( header->extended_length ),
    type( header->type ),
    four_byte_asn( f )
{
    auto len = header->len;
    bytes = std::vector<uint8_t>( header->data, header->data + len );
}

path_attr_t::path_attr_t( path_attr_header_extlen *header ):
    optional( header->optional ),
    transitive( header->transitive ),
    partial( header->partial ),
    extended_length( header->extended_length ),
    type( header->type )
{
    auto len = header->ext_len.native();
    bytes = std::vector<uint8_t>( header->data, header->data + len );
}

uint32_t path_attr_t::get_u32() const {
    return bswap( *reinterpret_cast<const uint32_t*>( bytes.data() ) );
}

std::vector<uint8_t> path_attr_t::to_bytes() const {
    std::vector<uint8_t> out;

    out.resize( sizeof( path_attr_header ) );
    auto header = reinterpret_cast<path_attr_header*>( out.data() );
    header->optional = optional;
    header->partial = partial;
    header->transitive = transitive;
    header->extended_length = extended_length;
    header->type = type;

    if( extended_length == 1 ) {
        out.resize( sizeof( path_attr_header_extlen ) );
        auto extlen_header = reinterpret_cast<path_attr_header_extlen*>( out.data() );
        extlen_header->ext_len = bytes.size();
    } else {
        header->len = bytes.size();
    }

    out.insert( out.end(), bytes.begin(), bytes.end() );

    return out;
}

std::vector<uint32_t> as_path_header::parse_be16() const {
    std::vector<uint32_t> list;
    auto p = &val16;
    for( int i = 0; i < len; i++ ) {
        list.emplace_back( p[i]->native() );
    }
    return list;
}
std::vector<uint32_t> as_path_header::parse_be32() const {
    std::vector<uint32_t> list;
    auto p = &val32;
    for( int i = 0; i < len; i++ ) {
        list.emplace_back( p[i]->native() );
    }
    return list;
}

std::vector<uint32_t> path_attr_t::parse_as_path() const {
    std::vector<uint32_t> list;

    int asn_size = four_byte_asn ? 4 : 2;

    auto temp = bytes;
    while( !temp.empty() ) {
        if( temp.size() < sizeof( as_path_header ) ) {
            temp.clear();
            break;
        }
        auto header = reinterpret_cast<as_path_header*>( temp.data() );
        if( temp.size() < ( sizeof( *header ) + asn_size * header->len ) ) {
            temp.clear();
            break;
        }
        std::vector<uint32_t> parsed_list;
        if( four_byte_asn ) {
            parsed_list = header->parse_be32();
        } else {
            parsed_list = header->parse_be32();
        }
        list.insert( list.end(), parsed_list.begin(), parsed_list.end() );
        temp.erase( temp.begin(), temp.begin() + sizeof( *header ) + ( header->len * asn_size ) );
    }

    return list;
}

bgp_cap_t::bgp_cap_t( const bgp_cap_header *cap ):
    code( cap->code ),
    data( (uint8_t*)cap->data, (uint8_t*)cap->data + cap->len )
{}

void bgp_cap_t::make_route_refresh() {
    data.clear();
    code = BGP_CAP_CODE::ROUTE_REFRESH;
}

void bgp_cap_t::make_fqdn( const std::string &host, const std::string &domain ) {
    data.clear();
    code = BGP_CAP_CODE::FQDN;
    data.insert( data.end(), host.size() );
    data.insert( data.end(), host.begin(), host.end() );
    data.insert( data.end(), domain.size() );
    data.insert( data.end(), domain.begin(), domain.end() );
}

void bgp_cap_t::make_4byte_asn( uint32_t asn ) {
    data.clear();
    code = BGP_CAP_CODE::FOUR_OCT_AS;
    data.resize( 4 );
    *reinterpret_cast<uint32_t*>( data.data() ) = bswap( asn );
}

void bgp_cap_t::make_mp_bgp( BGP_AFI afi ,BGP_SAFI safi ) {
    data.clear();
    code = BGP_CAP_CODE::MPBGP;
    data.resize( 4 );
    *reinterpret_cast<uint16_t*>( data.data() ) = bswap( static_cast<uint16_t>( afi ) );
    data[ 2 ] = 0;
    data[ 3 ] = static_cast<uint8_t>( safi );
}

bool bgp_cap_t::operator<( const bgp_cap_t &r ) const {
    return std::tie( code, data ) < std::tie( r.code, r.data );
}

std::vector<uint8_t> bgp_cap_t::toBytes() const {
    std::vector<uint8_t> out;

    out.reserve( sizeof( bgp_opt_param ) + sizeof( bgp_cap_header ) + data.size() );
    out.resize( sizeof( bgp_opt_param ) + sizeof( bgp_cap_header ) );
    auto param_head = reinterpret_cast<bgp_opt_param*>( out.data() );
    param_head->type = OPT_PARAM_TYPE::CAP;
    param_head->length = sizeof( bgp_cap_header ) + data.size();
    auto cap_head = reinterpret_cast<bgp_cap_header*>( (uint8_t*)param_head->value );
    cap_head->code = code;
    cap_head->len = data.size();
    out.insert( out.end(), data.begin(), data.end() );

    return out;
}

std::vector<bgp_cap_t> bgp_open::parse_capabilites() const {
    std::vector<bgp_cap_t> caps;

    auto offset = 0;
    while( offset < len ) {
        auto param_head = reinterpret_cast<const bgp_opt_param*>( data + offset );
        offset += param_head->length + sizeof( bgp_opt_param );
        if( param_head->type != OPT_PARAM_TYPE::CAP ) {
            continue;
        }
        auto cap_head = reinterpret_cast<const bgp_cap_header*>( (uint8_t*)param_head->value );
        caps.emplace_back( cap_head );
    }

    return caps;
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

uint8_t* bgp_packet::get_body() {
    return reinterpret_cast<uint8_t*>( data + sizeof( bgp_header ) );
}

std::tuple<std::vector<nlri>,std::vector<path_attr_t>,std::vector<nlri>> bgp_packet::process_update( bool four_byte_asn ) {
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
        uint16_t attr_len = 0;
        if( path->extended_length == 1 ) {
            auto extlen_path = reinterpret_cast<path_attr_header_extlen*>( path );
            paths.emplace_back( extlen_path );
            attr_len = sizeof( path_attr_header_extlen ) + extlen_path->ext_len.native();
        } else {
            paths.emplace_back( path, four_byte_asn );
            attr_len = sizeof( path_attr_header ) + path->len;
        }
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

void path_attr_t::make_local_pref( uint32_t val ) {
    val = bswap( val );
    type = PATH_ATTRIBUTE::LOCAL_PREF;
    bytes.resize( sizeof( val ) );
    std::memcpy( bytes.data(), &val, sizeof( val ) );
}

void path_attr_t::make_origin( ORIGIN o ) {
    type = PATH_ATTRIBUTE::ORIGIN;
    bytes.push_back( static_cast<uint8_t>( o ) );
}

void path_attr_t::make_nexthop( const address_v4 &a ) {
    type = PATH_ATTRIBUTE::NEXT_HOP;
    auto temp = a.to_bytes();
    bytes = { temp.begin(), temp.end() };
}

void path_attr_t::make_nexthop( const boost::asio::ip::address &a ) {
    type = PATH_ATTRIBUTE::NEXT_HOP;
    if( a.is_v4() ) {
        auto temp = a.to_v4().to_bytes();
        bytes = { temp.begin(), temp.end() };
    } else if( a.is_v6() ) {
        auto temp = a.to_v6().to_bytes();
        bytes = { temp.begin(), temp.end() };
    }
}