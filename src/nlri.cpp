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
    switch( afi ) {
    case BGP_AFI::IPv4:
        data.resize( 4 );
        if( inet_pton( AF_INET, prefix.c_str(), data.data() ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv4 prefix representation" );
        }
    case BGP_AFI::IPv6:
        data.resize( 16 );
        if( inet_pton( AF_INET6, prefix.c_str(), data.data() ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv6 prefix representation" );
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
    // char buf[ INET_ADDRSTRLEN ];
    // auto ret = inet_ntop( AF_INET, )
}

std::string NLRI::as_ipv6() const {

}