#include <boost/asio/ip/network_v4.hpp>
#include <yaml-cpp/yaml.h>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "yaml.hpp"
#include "config.hpp"

YAML::Node YAML::convert<GlobalConf>::encode(const GlobalConf& rhs) {
    Node node;
    node[ "listen_on_port" ]  = rhs.listen_on_port;
    node[ "my_as" ]           = rhs.my_as;
    node[ "hold_time" ]       = rhs.hold_time;
    node[ "bgp_router_id" ]   = rhs.bgp_router_id.to_string();
    node[ "neighbours" ]      = rhs.neighbours;
    return node;
}

bool YAML::convert<GlobalConf>::decode(const YAML::Node& node, GlobalConf& rhs) {
    // if(!node.IsSequence() || node.size() != 3) {
    //     return false;
    // }
    rhs.listen_on_port  = node[ "listen_on_port" ].as<uint16_t>();
    rhs.my_as           = node[ "my_as" ].as<uint32_t>();
    rhs.hold_time       = node[ "hold_time" ].as<uint16_t>();
    rhs.bgp_router_id   = address_v4::from_string( node["bgp_router_id"].as<std::string>() );
    rhs.neighbours      = node[ "neighbours" ].as<std::list<bgp_neighbour_v4>>();
    return true;
} 

YAML::Node YAML::convert<bgp_neighbour_v4>::encode(const bgp_neighbour_v4& rhs) {
    Node node;
    node[ "remote_as" ] = rhs.remote_as;
    node[ "address" ]   = rhs.address.to_string();
    if( rhs.hold_time.has_value() ) {
        node[ "hold_time" ] = *rhs.hold_time;
    }
    return node;
}

bool YAML::convert<bgp_neighbour_v4>::decode(const YAML::Node& node, bgp_neighbour_v4& rhs) {
    rhs.remote_as       = node["remote_as"].as<uint16_t>();
    rhs.address         = address_v4::from_string( node["address"].as<std::string>() );
    if( node[ "hold_time"].IsDefined() ) {
        rhs.hold_time       = node[ "hold_time" ].as<uint16_t>();
    }
    return true;
} 

YAML::Node YAML::convert<RoutePolicy>::encode(const RoutePolicy& rhs) {
    Node node;
    if( rhs.match_prefix_v4.has_value() ) {
        node[ "match_prefix_v4" ] = rhs.match_prefix_v4.value().to_string();
    }
    if( rhs.match_nexthop.has_value() ) {
        node[ "match_nexthop" ] = rhs.match_nexthop.value().to_string();
    }
    if( rhs.match_localpref.has_value() ) {
        node[ "match_localpref" ] = rhs.match_localpref.value();
    }
    if( rhs.set_nexthop.has_value() ) {
        node[ "set_nexthop" ] = rhs.set_nexthop.value().to_string();
    }
    if( rhs.set_localpref.has_value() ) {
        node[ "set_localpref" ] = rhs.set_localpref.value();
    }
    return node;
}

bool YAML::convert<RoutePolicy>::decode(const YAML::Node& node, RoutePolicy& rhs) {
    if( node[ "match_prefix_v4"].IsDefined() ) {
        rhs.match_prefix_v4.emplace( boost::asio::ip::make_network_v4( node[ "match_prefix_v4" ].as<std::string>() ) );
    }
    if( node[ "match_nexthop"].IsDefined() ) {
        rhs.match_nexthop.emplace( boost::asio::ip::make_address_v4( node[ "match_nexthop" ].as<std::string>() ) );
    }
    if( node[ "match_localpref"].IsDefined() ) {
        rhs.match_localpref.emplace( node[ "match_localpref" ].as<uint32_t>() );
    }
    if( node[ "set_nexthop"].IsDefined() ) {
        rhs.set_nexthop.emplace( boost::asio::ip::make_address_v4( node[ "set_nexthop" ].as<std::string>() ) );
    }
    if( node[ "set_localpref"].IsDefined() ) {
        rhs.set_localpref.emplace( node[ "set_localpref" ].as<uint32_t>() );
    }
    return true;
} 