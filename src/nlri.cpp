#include "nlri.hpp"

#include <stdexcept>
#include <arpa/inet.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "packet.hpp"

NLRI::NLRI( uint8_t *p, uint16_t len ):
    nlri_len( len )
{
    auto bytes = len / 8;
    if( len % 8 != 0 ) {
        bytes++;
    }
    data = std::vector<uint8_t>( p, p + bytes );
}

NLRI::NLRI( BGP_AFI afi, const std::string &prefix ) {
    if( auto it = prefix.find( '/' ); it != prefix.npos ) {
        auto substr = prefix.substr( it + 1 );
        nlri_len = std::stoi( substr );
    } else {
        throw std::runtime_error( "Can't find prefix len" );
    }
    auto bytes = nlri_len / 8;
    if( nlri_len % 8 != 0 ) {
        bytes++;
    }
    data.resize( bytes );
    switch( afi ) {
    case BGP_AFI::IPv4: {
        if( bytes > 4 ) {
            throw std::runtime_error( "Prefix len is too long" );
        }
        char buf[ 4 ];
        if( inet_pton( AF_INET, prefix.c_str(), buf ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv4 prefix representation" );
        }
        std::copy( buf, buf + bytes, data.data() );
    }
    case BGP_AFI::IPv6: {
        if( bytes > 16 ) {
            throw std::runtime_error( "Prefix len is too long" );
        }
        char buf[ 16 ];
        if( inet_pton( AF_INET6, prefix.c_str(), data.data() ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv6 prefix representation" );
        }
        std::copy( buf, buf + bytes, data.data() );
    }
    default:
        throw std::runtime_error( "Unknown AF" );
    }
}

std::vector<uint8_t> NLRI::serialize() const {
    std::vector<uint8_t> ret;

    ret.emplace_back( nlri_len );
    ret.insert( ret.end(), data.begin(), data.end() );

    return ret;
}

std::string NLRI::as_ipv4() const {
    char buf[ INET_ADDRSTRLEN ];
    std::array<uint8_t,4> address;
    std::fill( address.begin(), address.end(), 0 );
    std::copy( data.begin(), data.end(), address.begin() );

    auto ret = inet_ntop( AF_INET, address.data(), buf, sizeof( buf ) );
    if( ret != nullptr ) {
        return ret;
    } else {
        return {};
    }
}

std::string NLRI::as_ipv6() const {
    char buf[ INET6_ADDRSTRLEN ];
    std::array<uint8_t,16> address;
    std::fill( address.begin(), address.end(), 0 );
    std::copy( data.begin(), data.end(), address.begin() );

    auto ret = inet_ntop( AF_INET6, address.data(), buf, sizeof( buf ) );
    if( ret != nullptr ) {
        return ret;
    } else {
        return {};
    }
}