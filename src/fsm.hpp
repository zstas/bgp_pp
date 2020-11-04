#ifndef FSM_HPP_
#define FSM_HPP_

#include <list>
#include <set>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

using io_context = boost::asio::io_context;
using acceptor = boost::asio::ip::tcp::acceptor;
using endpoint = boost::asio::ip::tcp::endpoint;
using socket_tcp = boost::asio::ip::tcp::socket;
using error_code = boost::system::error_code;
using timer = boost::asio::steady_timer;
using stream_descriptor = boost::asio::posix::stream_descriptor;

using nlri = prefix_v4;

struct bgp_neighbour_v4;
struct GlobalConf;
struct bgp_packet;
struct bgp_cap_t;
enum class BGP_CAP_CODE : uint8_t;
enum class BGP_ERR_CODE : uint8_t;
enum class BGP_MSG_HDR_ERR : uint8_t;
enum class BGP_OPEN_ERR : uint8_t;
enum class BGP_UPDATE_ERR : uint8_t;
enum class BGP_FSM_ERR : uint8_t;
enum class BGP_CEASE_ERR : uint8_t;

#include "table.hpp"

enum class FSM_STATE {
    IDLE,
    CONNECT,
    ACTIVE,
    OPENSENT,
    OPENCONFIRM,
    ESTABLISHED,
};

struct bgp_fsm : public std::enable_shared_from_this<bgp_fsm> {
    FSM_STATE state;
    GlobalConf &gconf;
    bgp_neighbour_v4 &conf;
    bgp_table_v4 &table;
    std::vector<bgp_cap_t> caps;

    std::array<uint8_t,65535> buffer;
    std::optional<socket_tcp> sock;

    // counters
    uint64_t ConnectRetryCounter;

    // timers
    timer ConnectRetryTimer;
    timer HoldTimer;
    timer KeepaliveTimer;

    // config
    uint16_t ConnectRetryTime;
    uint16_t HoldTime;
    uint16_t KeepaliveTime;

    bgp_fsm( io_context &io, GlobalConf &g, bgp_table_v4 &t, bgp_neighbour_v4 &c );
    void place_connection( socket_tcp s );

    void start_keepalive_timer();
    void on_keepalive_timer( error_code ec );

    void on_receive( error_code ec, std::size_t length );
    void on_send( std::shared_ptr<std::vector<uint8_t>> pkt, error_code ec, std::size_t length );
    void do_read();

    void rx_open( bgp_packet &pkt );
    void tx_open( const std::set<bgp_cap_t> &caps );

    void rx_keepalive( bgp_packet &pkt );
    void tx_keepalive();

    void rx_update( bgp_packet &pkt );
    void tx_update( const std::vector<nlri> &prefixes, std::shared_ptr<std::vector<path_attr_t>> path, const std::vector<nlri> &withdrawn );

    void rx_notification( bgp_packet &pkt );
    void tx_notification( BGP_ERR_CODE code, BGP_MSG_HDR_ERR err, const std::vector<uint8_t> &data );
    void tx_notification( BGP_ERR_CODE code, BGP_OPEN_ERR err, const std::vector<uint8_t> &data );
    void tx_notification( BGP_ERR_CODE code, BGP_UPDATE_ERR err, const std::vector<uint8_t> &data );
    void tx_notification( BGP_ERR_CODE code, BGP_FSM_ERR err, const std::vector<uint8_t> &data );
    void tx_notification( BGP_ERR_CODE code, BGP_CEASE_ERR err, const std::vector<uint8_t> &data );
    void tx_notification( BGP_ERR_CODE code, uint8_t err, const std::vector<uint8_t> &data );

    void send_all_prefixes();
};

#endif