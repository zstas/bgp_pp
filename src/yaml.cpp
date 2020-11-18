#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <yaml-cpp/yaml.h>

using address_v4 = boost::asio::ip::address_v4;

#include "yaml.hpp"
#include "nlri.hpp"
#include "packet.hpp"
#include "config.hpp"

YAML::Node YAML::convert<GlobalConf>::encode(const GlobalConf& rhs) {
    Node node;
    node[ "listen_on_port" ]   = rhs.listen_on_port;
    node[ "my_as" ]            = rhs.my_as;
    node[ "hold_time" ]        = rhs.hold_time;
    node[ "bgp_router_id" ]    = rhs.bgp_router_id.to_string();
    node[ "neighbours" ]       = rhs.neighbours;
    node[ "originate_routes" ] = rhs.originate_routes;
    return node;
}

bool YAML::convert<GlobalConf>::decode(const YAML::Node& node, GlobalConf& rhs) {
    rhs.listen_on_port   = node[ "listen_on_port" ].as<uint16_t>();
    rhs.my_as            = node[ "my_as" ].as<uint32_t>();
    rhs.hold_time        = node[ "hold_time" ].as<uint16_t>();
    rhs.bgp_router_id    = address_v4::from_string( node["bgp_router_id"].as<std::string>() );
    rhs.neighbours       = node[ "neighbours" ].as<std::list<bgp_neighbour_v4>>();
    rhs.originate_routes = node[ "originate_routes" ].as<std::list<OrigEntry>>();
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
    node[ "entries" ] = rhs.entries;
    return node;
}

bool YAML::convert<RoutePolicy>::decode(const YAML::Node& node, RoutePolicy& rhs) {
    rhs.entries = node[ "entries" ].as<std::list<RoutePolicyEntry>>();
    return node;
}

YAML::Node YAML::convert<RoutePolicyEntry>::encode(const RoutePolicyEntry& rhs) {
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
    node[ "action" ] = rhs.action;
    return node;
}

bool YAML::convert<RoutePolicyEntry>::decode(const YAML::Node& node, RoutePolicyEntry& rhs) {
    rhs.action = node[ "action" ].as<RoutePolicyAction>();
    if( node[ "match_prefix_v4"].IsDefined() ) {
        rhs.match_prefix_v4.emplace( BGP_AFI::IPv4, node[ "match_prefix_v4" ].as<std::string>() );
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

YAML::Node YAML::convert<OrigEntry>::encode(const OrigEntry& rhs) {
    Node node;
    node[ "prefix" ] = rhs.prefix.to_string();
    if( rhs.policy_name.has_value() ) {
        node[ "policy_name" ] = rhs.policy_name.value();
    }
    return node;
}

bool YAML::convert<OrigEntry>::decode(const YAML::Node& node, OrigEntry& rhs) {
    rhs.prefix = NLRI( BGP_AFI::IPv4, node[ "prefix" ].as<std::string>() );
    if( node[ "policy_name"].IsDefined() ) {
        rhs.policy_name.emplace( node[ "policy_name" ].as<std::string>() );
    }
    return true;
} 

YAML::Node YAML::convert<RoutePolicyAction>::encode(const RoutePolicyAction& rhs) {
    Node node;
    switch( rhs ) {
    case RoutePolicyAction::ACCEPT:
        node = "accept";
        break;
    case RoutePolicyAction::DROP:
        node = "drop";
        break;
    case RoutePolicyAction::PASS:
        node = "pass";
        break;
    }
    return node;
}

bool YAML::convert<RoutePolicyAction>::decode(const YAML::Node& node, RoutePolicyAction& rhs) {
    if( node.as<std::string>() == "accept" ) {
        rhs = RoutePolicyAction::ACCEPT;
    } else if( node.as<std::string>() == "drop" ) {
        rhs = RoutePolicyAction::DROP;
    } else if( node.as<std::string>() == "pass" ) {
        rhs = RoutePolicyAction::PASS;
    }
    return true;
} 