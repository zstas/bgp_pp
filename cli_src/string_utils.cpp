#include <iostream>
#include <iomanip>
#include <vector>

#include "string_utils.hpp"
#include "message.hpp"

std::ostream& operator<<( std::ostream &os, const std::vector<uint8_t> &data ) {
    auto flags = os.flags();

    for( int b: data ) {
        os << "0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << b << " ";
    }

    os.flags( flags );
    return os;
}

std::ostream& operator<<( std::ostream &os, const TYPE &typ ) {
    switch( typ ) {
    case TYPE::REQ: os << "REQ"; break;
    case TYPE::RESP: os << "RESP"; break;
    default: os << "UNKNOWN"; break;
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

std::ostream& operator<<( std::ostream &os, const Message &msg ) {
    os << "Type: " << msg.type << std::endl;
    os << "Content: " << msg.cont;
    return os;
}

std::ostream& operator<<( std::ostream &os, const Show_Table_Req &msg ) {
    os << "Prefix: " << msg.prefix.value_or( "N/A" );
    return os;
}

std::ostream& operator<<( std::ostream &os, const Show_Table_Resp &msg ) {
    auto flags = os.flags();
    os << std::left << std::setw( 20 ) << "Prefix";
    os << std::setw( 16 ) << "Nexthop";
    os << std::setw( 16 ) << "LocalPref";
    os << std::setw( 30 ) << "Since";
    os << "AS Path";
    os << std::endl;
    for( auto const &entry: msg.entries ) {
        os << std::setw( 2 ) << ( entry.best ? '*' : ' ' );
        os << std::setw( 18 ) << entry.prefix;
        os << std::setw( 16 ) << entry.nexthop;
        os << std::setw( 16 ) << entry.local_pref;
        os << std::setw( 30 ) << entry.time;
        os << entry.as_path;
        os << std::endl;
    }
    os.flags( flags );
    return os;
}

std::ostream& operator<<( std::ostream &os, const Show_Neighbour_Resp &msg ) {
    auto flags = os.flags();
    os << std::left;
    os << std::setw( 20 ) << "Address";
    os << std::setw( 10 ) << "RemoteAS";
    os << std::setw( 10 ) << "HoldT";
    os << std::setw( 10 ) << "Sock";
    os << "Capabilites";
    os << std::endl;
    for( auto const &entry: msg.entries ) {
        os << std::setw( 18 ) << entry.address;
        os << std::setw( 10 ) << entry.remote_as;
        os << std::setw( 10 ) << entry.hold_time;
        os << std::setw( 10 );
        if( entry.socket.has_value() ) {
            os << entry.socket.value();
        } else {
            os << "N/A";
        }
        for( auto const &cap: entry.caps ) {
            os << cap << " ";
        }
        os << std::endl;
    }
    os.flags( flags );
    return os;
}