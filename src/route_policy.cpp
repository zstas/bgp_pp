#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "route_policy.hpp"
#include "nlri.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "table.hpp"

bool routePolicyProcess( RoutePolicy &pol, NLRI &nlri, bgp_path &path ) {
    if( pol.match_prefix_v4 ) {
        try {
            if( nlri != *pol.match_prefix_v4 ) {
                return false;
            }
        } catch( std::exception &e ) {
            return false;
        }
    }
    if( pol.match_nexthop ) {
        try {
            if( path.get_nexthop_v4() != *pol.match_nexthop ) {
                return false;
            }
        } catch( std::exception &e ) {
            return false;
        }
    }
    if( pol.match_localpref ) {
        try {
            if( path.get_local_pref() != *pol.match_localpref ) {
                return false;
            }
        } catch( std::exception &e ) {
            return false;
        }
    }
    if( pol.set_localpref ) {
        path.set_local_pref( *pol.set_localpref );
    }
    if( pol.set_nexthop ) {
        path.set_nexthop_v4( *pol.set_nexthop );
    }
    return true;
}