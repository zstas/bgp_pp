#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/address.hpp>

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

std::ostream& operator<<( std::ostream &os, const bgp_notification *notification ) {
    switch( notification->code ) {
    case BGP_ERR_CODE::MESSAGE_HEADER:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<BGP_MSG_HDR_ERR>( notification->subcode ) << std::endl;
        break;
    case BGP_ERR_CODE::OPEN_MESSAGE:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<BGP_OPEN_ERR>( notification->subcode ) << std::endl;
        break;
    case BGP_ERR_CODE::UPDATE_MESSAGE:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<BGP_UPDATE_ERR>( notification->subcode ) << std::endl;
        break;
    case BGP_ERR_CODE::HOLD_TIMER_EXPIRED:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<int>( notification->subcode ) << std::endl;
        break;
    case BGP_ERR_CODE::FSM_ERROR:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<BGP_FSM_ERR>( notification->subcode ) << std::endl;
        break;
    case BGP_ERR_CODE::CEASE:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<BGP_CEASE_ERR>( notification->subcode ) << std::endl;
        break;
    default:
        os << "code: " << notification->code << 
            "subcode: " << static_cast<int>( notification->subcode ) << std::endl;
        break;
    }
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

std::ostream& operator<<( std::ostream &os, const BGP_ERR_CODE &err ) {
    switch( err ) {
    case BGP_ERR_CODE::MESSAGE_HEADER:
        os << "Message Header";
        break;
    case BGP_ERR_CODE::OPEN_MESSAGE:
        os << "Open Message";
        break;
    case BGP_ERR_CODE::UPDATE_MESSAGE:
        os << "Update Message";
        break;
    case BGP_ERR_CODE::HOLD_TIMER_EXPIRED:
        os << "Hold Timer Expired";
        break;
    case BGP_ERR_CODE::FSM_ERROR:
        os << "FSM Error";
        break;
    case BGP_ERR_CODE::CEASE:
        os << "Cease";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_MSG_HDR_ERR &err ) {
    switch( err ) {
    case BGP_MSG_HDR_ERR::BAD_MSG_LENGTH:
        os << "Bad Message Length";
        break;
    case BGP_MSG_HDR_ERR::BAD_MSG_TYPE:
        os << "Bad Message Type";
        break;
    case BGP_MSG_HDR_ERR::CONN_NOT_SYNCH:
        os << "Connection not synchronized";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_OPEN_ERR &err ) {
    switch( err ) {
    case BGP_OPEN_ERR::BAD_BGP_ID:
        os << "Bad BGP ID";
        break;
    case BGP_OPEN_ERR::BAD_PEER_AS:
        os << "Bad Peer AS";
        break;
    case BGP_OPEN_ERR::UNACCEPTABLE_HOLD_TIME:
        os << "Unacceptable Hold Time";
        break;
    case BGP_OPEN_ERR::UNSUPPORTED_CAP:
        os << "Unsupported BGP Capability";
        break;
    case BGP_OPEN_ERR::UNSUPPORTED_OPT_PARAM:
        os << "Unsupported BGP Option";
        break;
    case BGP_OPEN_ERR::UNSUPPORTED_VERSION:
        os << "Unsupported BGP version";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_UPDATE_ERR &err ) {
    switch( err ) {
    case BGP_UPDATE_ERR::ATTR_FLAG_ERR:
        os << "Attribute Flag Error";
        break;
    case BGP_UPDATE_ERR::ATTR_LEN_ERR:
        os << "Attribute Length Error";
        break;
    case BGP_UPDATE_ERR::INV_NETWORK_FIELD:
        os << "Invalid Network Field";
        break;
    case BGP_UPDATE_ERR::INV_NHOP_ATTR:
        os << "Invalid NextHop Attribute";
        break;
    case BGP_UPDATE_ERR::INV_ORIGIN_ATTR:
        os << "Invalid Origin Attribute";
        break;
    case BGP_UPDATE_ERR::MALFORMED_AS_PATH:
        os << "Malformed AS_PATH";
        break;
    case BGP_UPDATE_ERR::MALFORMED_ATTR_LIST:
        os << "Malformed Attribute List";
        break;
    case BGP_UPDATE_ERR::MISSING_WELL_KNOWN_ATTR:
        os << "Missing Well-Known Attribute";
        break;
    case BGP_UPDATE_ERR::OPT_ATTR_ERR:
        os << "Optional Attribute Error";
        break;
    case BGP_UPDATE_ERR::UNRECOGNIZED_WELL_KNOWN_ATTR:
        os << "Unrecognized Well-Known Attribute";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_FSM_ERR &err ) {
    switch( err ) {
    case BGP_FSM_ERR::UNEXP_ESTABLISHED:
        os << "Unexpected Packet in Established state";
        break;
    case BGP_FSM_ERR::UNEXP_OPENCONF:
        os << "Unexpected Packet in OpenConf state";
        break;
    case BGP_FSM_ERR::UNEXP_OPENSENT:
        os << "Unexpected Packet in OpenSent state";
        break;
    case BGP_FSM_ERR::UNSPEC_ERR:
        os << "Unspecified Error";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}

std::ostream& operator<<( std::ostream &os, const BGP_CEASE_ERR &err ) {
    switch( err ) {
    case BGP_CEASE_ERR::ADM_RESET:
        os << "Administrative Reset";
        break;
    case BGP_CEASE_ERR::ADM_SHUTDOWN:
        os << "Administrative Shutdown";
        break;
    case BGP_CEASE_ERR::CONN_COLLISION_RES:
        os << "Connection Collision Resolution";
        break;
    case BGP_CEASE_ERR::CONN_REJECTED:
        os << "Connection Rejected";
        break;
    case BGP_CEASE_ERR::HARD_RESET:
        os << "Hard Reset";
        break;
    case BGP_CEASE_ERR::MAX_PREFIXES_REACHED:
        os << "Maximum Prefixes Reached";
        break;
    case BGP_CEASE_ERR::OTH_CONF_CHANGE:
        os << "Other Configuration Changed";
        break;
    case BGP_CEASE_ERR::OUT_OF_RES:
        os << "Out Of Resources";
        break;
    case BGP_CEASE_ERR::PEER_DECONF:
        os << "Peer Deconfigured";
        break;
    default:
        os << "Unknown (" << static_cast<int>( err ) << ")";
        break;
    }
    return os;
}