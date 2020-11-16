#ifndef ROUTE_POLICY_HPP
#define ROUTE_POLICY_HPP

#include <vector>
struct RoutePolicy;
struct path_attr_t;
struct bgp_path;
class NLRI;

bool routePolicyProcess( RoutePolicy &pol, NLRI &nlri, bgp_path &path );

#endif