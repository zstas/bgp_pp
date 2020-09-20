#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <list>

struct bgp_neighbour_v4 {
    uint32_t remote_as;
    address_v4 address;
    std::optional<uint16_t> hold_time;
};

struct GlobalConf {
    uint16_t listen_on_port;
    uint32_t my_as;
    address_v4 bgp_router_id;
    uint16_t hold_time;

    std::list<bgp_neighbour_v4> neighbours;
};

#endif