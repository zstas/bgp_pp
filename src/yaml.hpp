#ifndef YAML_HPP
#define YAML_HPP

struct GlobalConf;
struct bgp_neighbour_v4;
struct RoutePolicy;
struct RoutePolicyEntry;
enum RoutePolicyAction: uint8_t;
struct OrigEntry;

namespace YAML {
    template<>
    struct convert<bgp_neighbour_v4> {
        static Node encode( const bgp_neighbour_v4 &rhs );
        static bool decode( const Node& node, bgp_neighbour_v4 &rhs );
    };

    template<>
    struct convert<GlobalConf> {
        static Node encode( const GlobalConf &rhs );
        static bool decode( const Node &node, GlobalConf &rhs );
    };

    template<>
    struct convert<RoutePolicy> {
        static Node encode( const RoutePolicy &rhs );
        static bool decode( const Node &node, RoutePolicy &rhs );
    };

    template<>
    struct convert<RoutePolicyEntry> {
        static Node encode( const RoutePolicyEntry &rhs );
        static bool decode( const Node &node, RoutePolicyEntry &rhs );
    };

    template<>
    struct convert<RoutePolicyAction> {
        static Node encode( const RoutePolicyAction &rhs );
        static bool decode( const Node &node, RoutePolicyAction &rhs );
    };

    template<>
    struct convert<OrigEntry> {
        static Node encode( const OrigEntry &rhs );
        static bool decode( const Node &node, OrigEntry &rhs );
    };
}

#endif