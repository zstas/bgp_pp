#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "route_policy.hpp"
#include "nlri.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "table.hpp"

bool routePolicyProcess( RoutePolicy &pol, NLRI &nlri, std::vector<path_attr_t> &attrs ) {
    for( auto const &entry: pol.entries ) {
        if( entry.match_prefix_v4 ) {
            if( nlri != *entry.match_prefix_v4 ) {
                continue;
            }
        }
        for( auto &attr: attrs ) {
            if( attr.type == PATH_ATTRIBUTE::NEXT_HOP && entry.match_nexthop ) {
                if( attr.get_u32() != entry.match_nexthop->to_uint() ) {
                    continue;
                }
            }
            if( attr.type == PATH_ATTRIBUTE::LOCAL_PREF && entry.match_localpref ) {
                if( attr.get_u32() != *entry.match_localpref ) {
                    continue;
                }
            }
            if( attr.type == PATH_ATTRIBUTE::LOCAL_PREF && entry.set_localpref ) {
                attr.make_local_pref( *entry.set_localpref );
            }
            if( attr.type == PATH_ATTRIBUTE::NEXT_HOP && entry.set_nexthop ) {
                attr.make_nexthop( *entry.set_nexthop );
            }
            if( entry.action == RoutePolicyAction::ACCEPT ) {
                return true;
            } else if( entry.action == RoutePolicyAction::PASS ) {
                continue;
            } else if( entry.action == RoutePolicyAction::DROP ) {
                return false;
            }
        }
    }
    return false;
}