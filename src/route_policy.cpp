#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "route_policy.hpp"
#include "nlri.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "table.hpp"

bool routePolicyProcess( RoutePolicy &pol, NLRI &nlri, std::vector<path_attr_t> &attrs ) {
    if( pol.match_prefix_v4 ) {
        try {
            if( nlri != *pol.match_prefix_v4 ) {
                return false;
            }
        } catch( std::exception &e ) {
            return false;
        }
    }
    for( auto &attr: attrs ) {
        if( attr.type == PATH_ATTRIBUTE::NEXT_HOP && pol.match_nexthop ) {
            if( attr.get_u32() != pol.match_nexthop->to_uint() ) {
                return false;
            }
        }
        if( attr.type == PATH_ATTRIBUTE::LOCAL_PREF && pol.match_localpref ) {
            if( attr.get_u32() != *pol.match_localpref ) {
                return false;
            }
        }
        if( attr.type == PATH_ATTRIBUTE::LOCAL_PREF && pol.set_localpref ) {
            attr.make_local_pref( *pol.set_localpref );
        }
        if( attr.type == PATH_ATTRIBUTE::NEXT_HOP && pol.set_nexthop ) {
            attr.make_nexthop( *pol.set_nexthop );
        }
    }
    return true;
}