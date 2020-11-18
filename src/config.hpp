#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <list>
#include "nlri.hpp"

struct bgp_neighbour_v4 {
    uint32_t remote_as;
    address_v4 address;
    std::optional<uint16_t> hold_time;
};

enum RoutePolicyAction: uint8_t {
    ACCEPT,
    DROP,
    PASS
};

struct RoutePolicyEntry {
    // match
    std::optional<NLRI> match_prefix_v4;
    std::optional<address_v4> match_nexthop;
    std::optional<uint32_t> match_localpref;

    // action
    std::optional<address_v4> set_nexthop;
    std::optional<uint32_t> set_localpref;
    RoutePolicyAction action;
};

struct RoutePolicy {
    std::list<RoutePolicyEntry> entries;
};

struct OrigEntry {
    NLRI prefix;
    std::optional<std::string> policy_name;
};

struct GlobalConf {    
    uint16_t listen_on_port;
    uint32_t my_as;
    address_v4 bgp_router_id;
    uint16_t hold_time;

    std::list<bgp_neighbour_v4> neighbours;
    std::list<OrigEntry> originate_routes;
    std::map<std::string,RoutePolicy> policies;
};

#endif