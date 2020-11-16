#ifndef PACKET_HPP_
#define PACKET_HPP_

#include "net_integer.hpp"

class NLRI;

enum class PATH_ATTRIBUTE : uint8_t {
    ORIGIN = 1,
    AS_PATH = 2,
    NEXT_HOP = 3,
    MULTI_EXIT_DISC = 4,
    LOCAL_PREF = 5,
    ATOMIC_AGGREGATE = 6,
    AGGREGATOR = 7,
};

enum class ORIGIN : uint8_t {
    IGP = 0,
    EGP = 1,
    INCOMPLETE = 2,
};

enum class AS_PATH_SEGMENT_TYPE : uint8_t {
    AS_SET = 1,
    AS_SEQUENCE = 2,
};

enum class OPT_PARAM_TYPE : uint8_t {
    CAP = 2,
};

enum class BGP_CAP_CODE : uint8_t {
    MPBGP = 1,
    ROUTE_REFRESH = 2,
    OUTBOUND_ROUTE_FILTERING = 3,
    ENH_NH_ENCODING = 5,
    BGP_EXT_MESSAGE = 6,
    BGP_SEC = 7,
    MULTIPLE_LABELS = 8,
    GRACEFUL_RESTART = 64,
    FOUR_OCT_AS = 65,
    ADD_PATH = 69,
    ENH_ROUTE_REFRESH = 70,
    FQDN = 73,
};

enum class BGP_AFI : uint16_t {
    IPv4 = 1,
    IPv6 = 2,
    L2VPN = 25,
    BGP_LS = 16388,
};

enum class BGP_SAFI : uint8_t {
    UNICAST = 1,
    MULTICAST = 2,
};

enum class BGP_ERR_CODE : uint8_t {
    MESSAGE_HEADER = 1,
    OPEN_MESSAGE = 2,
    UPDATE_MESSAGE = 3,
    HOLD_TIMER_EXPIRED = 4,
    FSM_ERROR = 5,
    CEASE = 6,
};

enum class BGP_MSG_HDR_ERR : uint8_t {
    CONN_NOT_SYNCH = 1,
    BAD_MSG_LENGTH = 2,
    BAD_MSG_TYPE = 3,
};

enum class BGP_OPEN_ERR : uint8_t {
    UNSUPPORTED_VERSION = 1,
    BAD_PEER_AS = 2,
    BAD_BGP_ID = 3,
    UNSUPPORTED_OPT_PARAM = 4,
    // DEPRECATED
    UNACCEPTABLE_HOLD_TIME = 6,
    UNSUPPORTED_CAP = 7,
};

enum class BGP_UPDATE_ERR : uint8_t {
    MALFORMED_ATTR_LIST = 1,
    UNRECOGNIZED_WELL_KNOWN_ATTR = 2,
    MISSING_WELL_KNOWN_ATTR = 3,
    ATTR_FLAG_ERR = 4,
    ATTR_LEN_ERR = 5,
    INV_ORIGIN_ATTR = 6,
    // DEPRECATED
    INV_NHOP_ATTR = 8,
    OPT_ATTR_ERR = 9,
    INV_NETWORK_FIELD = 10,
    MALFORMED_AS_PATH = 11,
};

enum class BGP_FSM_ERR : uint8_t {
    UNSPEC_ERR = 0,
    UNEXP_OPENSENT = 1,
    UNEXP_OPENCONF = 2,
    UNEXP_ESTABLISHED = 3,
};

enum class BGP_CEASE_ERR : uint8_t {
    MAX_PREFIXES_REACHED = 1,
    ADM_SHUTDOWN = 2,
    PEER_DECONF = 3,
    ADM_RESET = 4,
    CONN_REJECTED = 5,
    OTH_CONF_CHANGE = 6,
    CONN_COLLISION_RES = 7,
    OUT_OF_RES = 8,
    HARD_RESET = 9,
};

struct as_path_header {
    AS_PATH_SEGMENT_TYPE type;
    uint8_t len;
    BE16 val16[0];
    BE32 val32[0];

    std::vector<uint32_t> parse_be16() const;
    std::vector<uint32_t> parse_be32() const;
}__attribute__((__packed__));

static_assert( sizeof( as_path_header ) == 2, "size of as_path_header should be equal 2 bytes" );

struct path_attr_header {
    uint8_t unused:4;
    uint8_t extended_length:1;
    uint8_t partial:1;
    uint8_t transitive:1;
    uint8_t optional:1;
    PATH_ATTRIBUTE type;
    uint8_t len;
    uint8_t data[0];
}__attribute__((__packed__));

static_assert( sizeof( path_attr_header ) == 3, "size of path_attr_header should be equal 2 bytes" );

struct path_attr_header_extlen {
    uint8_t unused:4;
    uint8_t extended_length:1;
    uint8_t partial:1;
    uint8_t transitive:1;
    uint8_t optional:1;
    PATH_ATTRIBUTE type;
    BE16 ext_len;
    uint8_t data[0];
}__attribute__((__packed__));

static_assert( sizeof( path_attr_header_extlen ) == 4, "size of path_attr_header_extlen should be equal 2 bytes" );

struct path_attr_t {
    uint8_t optional:1;
    uint8_t transitive:1;
    uint8_t partial:1;
    uint8_t extended_length:1;
    uint8_t unused:4;
    PATH_ATTRIBUTE type;
    std::vector<uint8_t> bytes;
    bool four_byte_asn;

    path_attr_t() = default;
    path_attr_t( path_attr_header *header, bool four_byte_asn = false );
    path_attr_t( path_attr_header_extlen *header );

    void make_local_pref( uint32_t val );
    void make_origin( ORIGIN o );
    void make_nexthop( const NLRI &a );
    void make_nexthop( const boost::asio::ip::address &a );
    void make_nexthop( const address_v4 &a );
    void make_as_path( std::vector<uint32_t> aspath );

    uint32_t get_u32() const;
    std::vector<uint32_t> parse_as_path() const;
    std::vector<uint8_t> to_bytes() const;
};

bool operator==( const path_attr_t &lhs, const path_attr_t &rhs );

enum class bgp_type : uint8_t {
    OPEN = 1,
    UPDATE = 2,
    NOTIFICATION = 3,
    KEEPALIVE = 4,
    ROUTE_REFRESH = 5,
};

struct bgp_header {
    std::array<uint8_t,16> marker;
    BE16 length;
    bgp_type type;
}__attribute__((__packed__));

struct bgp_opt_param {
    OPT_PARAM_TYPE type;
    uint8_t length;
    uint8_t value[0];
}__attribute__((__packed__));

struct bgp_cap_header {
    BGP_CAP_CODE code;
    uint8_t len;
    uint8_t data[0];
}__attribute__((__packed__));

static_assert( sizeof( bgp_cap_header ) == 2, "bgp_cap header should be 2 bytes" );

struct bgp_cap_t {
    bgp_cap_t() = default;
    bgp_cap_t( const bgp_cap_header *cap );
    BGP_CAP_CODE code;
    std::vector<uint8_t> data;

    bool operator<( const bgp_cap_t &r ) const;
    void make_route_refresh();
    void make_fqdn( const std::string &host, const std::string &domain );
    void make_4byte_asn( uint32_t asn );
    void make_mp_bgp( BGP_AFI afi ,BGP_SAFI safi );
    std::vector<uint8_t> toBytes() const;
};

struct bgp_open {
    uint8_t version;
    BE16 my_as;
    BE16 hold_time;
    BE32 bgp_id;
    uint8_t len;
    uint8_t data[0];
    std::vector<bgp_cap_t> parse_capabilites() const;
}__attribute__((__packed__));

struct bgp_notification {
    BGP_ERR_CODE code;
    uint8_t subcode;
    uint8_t data[0];
}__attribute__((__packed__));

struct bgp_packet {
    bgp_header *header = nullptr;

    uint8_t *data;
    std::size_t length;

    bgp_packet( uint8_t *begin, std::size_t l );
    bgp_header* get_header();
    bgp_open* get_open();
    bgp_notification* get_notification();
    uint8_t* get_body();
    std::tuple<std::vector<NLRI>,std::vector<path_attr_t>,std::vector<NLRI>> process_update( bool four_byte_asn );
};

#endif