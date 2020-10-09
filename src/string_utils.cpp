#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/asio/ip/network_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "string_utils.hpp"
#include "log.hpp"
#include "config.hpp"
#include "packet.hpp"
#include "message.hpp"

std::ostream& operator<<( std::ostream &os, const LOGL &l ) {
    switch( l ) {
    case LOGL::TRACE: return os << "[TRACE] ";
    case LOGL::DEBUG: return os << "[DEBUG] ";
    case LOGL::INFO: return os << "[INFO] ";
    case LOGL::WARN: return os << "[WARN] ";
    case LOGL::ERROR: return os << "[ERROR] ";
    case LOGL::ALERT: return os << "[ALERT] ";
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const LOGS &l ) {
    switch( l ) {
    case LOGS::MAIN: return os << "[MAIN] ";
    case LOGS::PACKET: return os << "[PACKET] ";
    case LOGS::BGP: return os << "[BGP] ";
    case LOGS::EVENT_LOOP: return os << "[EVENT LOOP] ";
    case LOGS::FSM: return os << "[FSM] ";
    case LOGS::CONNECTION: return os << "[CONNECTION] ";
    case LOGS::CONFIGURATION: return os << "[CONFIG] ";
    case LOGS::CLI: return os << "[CLI] ";
    case LOGS::TABLE: return os << "[TABLE] ";
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const std::vector<uint8_t> &val ) {
    auto flags = os.flags();

    for( auto const &b: val ) {
        os << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( b ) << " ";
    }

    os.flags( flags );
    return os;
}

std::ostream& operator<<( std::ostream &os, const bgp_neighbour_v4 &nei ) {
    os << "Neighbour: " << nei.address.to_string();
    os << " Remote AS: " << nei.remote_as;
    if( nei.hold_time.has_value() ) {
        os << " Hold Time: " << nei.hold_time.value();
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const GlobalConf &conf ) {
    os << "My AS: " << conf.my_as << std::endl;
    os << "Listen on port: " << conf.listen_on_port << std::endl;
    os << "BGP Router ID: " << conf.bgp_router_id.to_string() << std::endl;
    for( auto const &n: conf.neighbours ) {
        os << n << std::endl;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const bgp_open *open ) {
    os << "BGP version: " << static_cast<int>( open->version );
    os << ", Router ID: " << address_v4( open->bgp_id.native() ).to_string();
    os << ", Hold time: " << open->hold_time.native();
    os << ", AS: " << open->my_as.native();
    return os;
}

std::ostream& operator<<( std::ostream &os, const path_attr_t &attr ) {
    os << "Type: " << attr.type;
    os << " Length: " << attr.bytes.size();
    os << " Value: ";
    switch( attr.type ) {
    case PATH_ATTRIBUTE::ORIGIN:
        os << static_cast<ORIGIN>( attr.bytes[0] );
        break;
    case PATH_ATTRIBUTE::NEXT_HOP: 
        os << address_v4( attr.get_u32() ).to_string();
        break;
    case PATH_ATTRIBUTE::LOCAL_PREF:
        os << attr.get_u32();
        break;
    case PATH_ATTRIBUTE::MULTI_EXIT_DISC:
        os << attr.get_u32();
        break;
    case PATH_ATTRIBUTE::AS_PATH:
        for( auto const &l : attr.parse_as_path() ) {
            os << l << " ";
        }
        break;
    default:
        os << "NA";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const PATH_ATTRIBUTE &attr ) {
    switch( attr ) {
    case PATH_ATTRIBUTE::ORIGIN:
        os << "ORIGIN"; break;
    case PATH_ATTRIBUTE::AS_PATH:
        os << "AS_PATH"; break;
    case PATH_ATTRIBUTE::NEXT_HOP:
        os << "NEXT_HOP"; break;
    case PATH_ATTRIBUTE::MULTI_EXIT_DISC:
        os << "MULTI_EXIT_DISC"; break;
    case PATH_ATTRIBUTE::LOCAL_PREF:
        os << "LOCAL_PREF"; break;
    case PATH_ATTRIBUTE::ATOMIC_AGGREGATE:
        os << "ATOMIC_AGGREGATE"; break;
    case PATH_ATTRIBUTE::AGGREGATOR:
        os << "AGGREGATOR"; break;
    default:
        os << "NA"; break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const ORIGIN &orig ) {
    switch( orig ) {
    case ORIGIN::EGP:
        os << "EGP"; break;
    case ORIGIN::IGP:
        os << "IGP"; break;
    case ORIGIN::INCOMPLETE:
        os << "INCOMPLETE"; break;
    default:
        os << "ERROR"; break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const CONTENT &cont ) {
    switch( cont ) {
    case CONTENT::SHOW_VER: os << "SHOW_VER"; break;
    case CONTENT::SHOW_TABLE: os << "SHOW_TABLE"; break;
    case CONTENT::SHOW_NEI: os << "SHOW_NEI"; break;
    default: os << "UNKNOWN"; break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_CAP_CODE &cap ) {
    switch( cap ) {
    case BGP_CAP_CODE::MPBGP: os << "MPBGP"; break;
    case BGP_CAP_CODE::ROUTE_REFRESH: os << "ROUTE_REFRESH"; break;
    case BGP_CAP_CODE::OUTBOUND_ROUTE_FILTERING: os << "OUTBOUND_ROUTE_FILTERING"; break;
    case BGP_CAP_CODE::ENH_NH_ENCODING: os << "ENH_NH_ENCODING"; break;
    case BGP_CAP_CODE::BGP_EXT_MESSAGE: os << "BGP_EXT_MESSAGE"; break;
    case BGP_CAP_CODE::BGP_SEC: os << "BGP_SEC"; break;
    case BGP_CAP_CODE::MULTIPLE_LABELS: os << "MULTIPLE_LABELS"; break;
    case BGP_CAP_CODE::GRACEFUL_RESTART: os << "GRACEFUL_RESTART"; break;
    case BGP_CAP_CODE::FOUR_OCT_AS: os << "FOUR_OCT_AS"; break;
    case BGP_CAP_CODE::ADD_PATH: os << "ADD_PATH"; break;
    case BGP_CAP_CODE::ENH_ROUTE_REFRESH: os << "ENH_ROUTE_REFRESH"; break;
    case BGP_CAP_CODE::FQDN: os << "FQDN"; break;
    default: os << "Unknown capability with code " << static_cast<int>( cap ) ; break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const bgp_cap_t &cap ) {
    os << "CAP: " << cap.code;
    os << " Value: ";
    switch( cap.code ) {
    case BGP_CAP_CODE::FQDN: {
        auto host_it = cap.data.begin();
        auto host = std::string( host_it + 1, host_it + 1 + *host_it );
        auto domain_it = cap.data.begin() + 1 + *host_it;
        auto domain = std::string( domain_it + 1, domain_it + 1 + *domain_it );
        os << "Host: " << host << " Domain: " << domain;
        break;
    }
    default:
        os << cap.data;
    }
    return os;
}