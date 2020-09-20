#ifndef YAML_HPP
#define YAML_HPP

struct GlobalConf;
struct bgp_neighbour_v4;

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
}

#endif