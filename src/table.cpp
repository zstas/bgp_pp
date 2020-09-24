#include <boost/asio/ip/network_v4.hpp>
#include <map>
#include <chrono>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "table.hpp"
#include "fsm.hpp"
#include "packet.hpp"

using prefix_v4 = boost::asio::ip::network_v4;

bgp_path::bgp_path( std::vector<path_attr_t> a, std::shared_ptr<bgp_fsm> s ):
    attrs( std::move( a ) ),
    source( std::move( s ) ),
    time( std::chrono::system_clock::now() )
{}

void bgp_table_v4::add_path( prefix_v4 prefix, std::vector<path_attr_t> attr, std::shared_ptr<bgp_fsm> nei ) {
    auto it = std::find_if( 
        table.begin(), 
        table.end(), 
        [ &attr ]( const std::pair<prefix_v4,std::shared_ptr<bgp_path>> &pair ) -> bool {
            return pair.second->attrs == attr;
        }
    );
    if( it == table.end() ) {
        table.emplace( std::piecewise_construct,
            std::forward_as_tuple( prefix ), 
            std::forward_as_tuple( std::make_shared<bgp_path>( attr, nei ) )
        );
    } else {
        table.emplace( std::piecewise_construct, 
            std::forward_as_tuple( prefix ), 
            std::forward_as_tuple( it->second )
        );
    }
}

void bgp_table_v4::del_path( prefix_v4 prefix, std::shared_ptr<bgp_fsm> nei ) {
    auto const &range = table.equal_range( prefix );
    for( auto i = range.first; i != range.second; ++i ) {
        if( i->second->source == nei ) {
            table.erase( i );
            break;
        }
    }
}