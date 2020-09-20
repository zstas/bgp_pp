#include <boost/asio/ip/network_v4.hpp>
#include <map>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "table.hpp"
#include "packet.hpp"

using prefix_v4 = boost::asio::ip::network_v4;

void bgp_table_v4::add_path( prefix_v4 prefix, std::vector<path_attr_t> attr ) {
    table.emplace( prefix, attr );
}

void bgp_table_v4::del_path( prefix_v4 prefix, std::vector<path_attr_t> attr ) {
    auto const &range = table.equal_range( prefix );
    for( auto i = range.first; i != range.second; ++i ) {
        if( i->second == attr ) {
            table.erase( i );
            break;
        }
    }
}