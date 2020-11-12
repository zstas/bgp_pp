#include "nlri.hpp"

#include <stdexcept>
#include <arpa/inet.h>
#include <sstream>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "packet.hpp"

NLRI::NLRI( BGP_AFI a, uint8_t *p, uint8_t len  ):
    afi( a ),
    nlri_len( len )
{
    auto bytes = nlri_len / 8;
    if( nlri_len % 8 != 0 ) {
        bytes++;
    }
    data = std::vector<uint8_t>( p, p + bytes );
}

NLRI::NLRI( BGP_AFI a, const std::string &prefix ):
    afi( a ) 
{
    std::string address;
    if( auto it = prefix.find( '/' ); it != prefix.npos ) {
        nlri_len = std::stoi( prefix.substr( it + 1 ) );
        address = prefix.substr( 0, it );
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
        if( inet_pton( AF_INET, address.c_str(), buf ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv4 prefix representation" );
        }
        std::copy( buf, buf + bytes, data.data() );
        break;
    }
    case BGP_AFI::IPv6: {
        if( bytes > 16 ) {
            throw std::runtime_error( "Prefix len is too long" );
        }
        char buf[ 16 ];
        if( inet_pton( AF_INET6, address.c_str(), buf ) != 1 ) {
            throw std::runtime_error( "Cannot convert from IPv6 prefix representation" );
        }
        std::copy( buf, buf + bytes, data.data() );
        break;
    }
    default:
        throw std::runtime_error( "Unknown AF" );
    }
}

bool operator<( const NLRI &lhv,const NLRI &rhv ) {
    return std::tie( lhv.afi, lhv.data, lhv.nlri_len ) < std::tie( rhv.afi, rhv.data, rhv.nlri_len );
}

bool operator==( const NLRI &lhv,const NLRI &rhv ) {
    return std::tie( lhv.afi, lhv.data, lhv.nlri_len ) == std::tie( rhv.afi, rhv.data, rhv.nlri_len );
}

bool operator!=( const NLRI &lhv,const NLRI &rhv ) {
    return !( lhv == rhv );
}

std::vector<uint8_t> NLRI::serialize() const {
    std::vector<uint8_t> ret;

    ret.emplace_back( nlri_len );
    ret.insert( ret.end(), data.begin(), data.end() );

    return ret;
}

std::string NLRI::to_string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<<( std::ostream &os, const NLRI &n ) {
    switch( n.afi ) {
    case BGP_AFI::IPv4: {
        char buf[ INET_ADDRSTRLEN ];
        std::array<uint8_t,4> address;
        std::fill( address.begin(), address.end(), 0 );
        std::copy( n.data.begin(), n.data.end(), address.begin() );
        if( auto ret = inet_ntop( AF_INET, address.data(), buf, sizeof( buf ) ); ret != nullptr ) {
            os << buf << "/" << static_cast<int>( n.nlri_len );
        } else {
            os << "Invalid Data";
        }
        break;
    }
    case BGP_AFI::IPv6: {
        char buf[ INET6_ADDRSTRLEN ];
        std::array<uint8_t,16> address;
        std::fill( address.begin(), address.end(), 0 );
        std::copy( n.data.begin(), n.data.end(), address.begin() );
        if( auto ret = inet_ntop( AF_INET6, address.data(), buf, sizeof( buf ) ); ret != nullptr ) {
            os << buf << "/" << static_cast<int>( n.nlri_len );
        } else {
            os << "Invalid Data";
        }
        break;
    }
    default:
        os << "Unknown AFI ";
        for( auto const &b : n.data ) {
            os << static_cast<int>( b ) << ":"; 
        }
        os << "\b" << "/" << static_cast<int>( n.nlri_len );
    }
    return os;
}