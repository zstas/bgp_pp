#ifndef TABLE_HPP_
#define TABLE_HPP_

struct path_attr_t;

namespace std {
template<>
struct less<prefix_v4> {
    bool operator() (const prefix_v4& lhs, const prefix_v4& rhs) const
    {
        return ( lhs.address() < rhs.address() ) || ( lhs.prefix_length() < rhs.prefix_length() );
    }
};
}

struct bgp_table_v4 {
    std::multimap<prefix_v4,std::vector<path_attr_t>> table;
    void add_path( prefix_v4 prefix, std::vector<path_attr_t> attr );
    void del_path( prefix_v4 prefix, std::vector<path_attr_t> attr );
};

#endif