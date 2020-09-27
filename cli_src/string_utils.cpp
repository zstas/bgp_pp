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

std::ostream& operator<<( std::ostream &os, const Show_Table &msg ) {
    os << "Prefix: " << msg.prefix.value_or( "N/A" );
    return os;
}