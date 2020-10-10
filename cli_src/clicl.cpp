#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "clicl.hpp"
#include "message.hpp"
#include "string_utils.hpp"

// void CLI_Token::lookup( std::list<std::string> &cmd, std::map<std::string,std::string> &args ) {
//     if( cmd.empty() ) {
//         if( tok != TOKEN::END ) {
//             throw std::runtime_error( "Invalid input" );
//         } else {
//             callback( args );
//         }
//     }
//     std::string cur = cmd.front();
//     cmd.erase( cmd.begin() );
//     if( tok == TOKEN::ARG ) {
//         args.emplace( data, cur );
//         auto it = next_attrs.begin();
//         it->lookup( cmd, args );
//     } else if( tok == TOKEN::CONT ) {
//         auto it = std::find( next_attrs.begin(), next_attrs.end(), cur );
//         if( it == next_attrs.end() ) {
//             throw std::runtime_error( "Invalid input" );
//         }
//         it->lookup( cmd, args );
//     }
// }

CLI_Client::CLI_Client( boost::asio::io_context &i, const std::string &path ):
    io( i ),
    ep( path ),
    sock( io ),
    input( io, STDIN_FILENO )
{
    map.emplace( "show version", CONTENT::SHOW_VER );
    map.emplace( "show table", CONTENT::SHOW_TABLE );
    map.emplace( "show neighbour", CONTENT::SHOW_NEI );
    sock.async_connect( ep, std::bind( &CLI_Client::on_connect, this, std::placeholders::_1 ) );
    std::cout << "Connecting to bgp daemon..." << std::endl;
}

void CLI_Client::on_connect( const boost::system::error_code &ec ) {
    if( ec ) {
        std::cerr << ec.message() << std::endl;
        exit( -1 );
    }
    std::cout << "Connected" << std::endl;
    read_cli_cmd();
}

std::string CLI_Client::sync_send( std::string out ) {
    sock.send( boost::asio::buffer( out ) );
    auto len = sock.receive( boost::asio::buffer( buf ) );
    return { buf.data(), buf.data() + len };
}

void CLI_Client::read_cli_cmd() {
    std::cout << "bgp# ";
    std::cout.flush();
    boost::system::error_code ec;
    auto len = input.read_some( boost::asio::buffer( buf ), ec );
    if( ec ) {
        std::cerr << ec.message() << std::endl;
        exit( -1 );
    }
    std::string in { buf.data(), buf.data() + len - 1 };
    try{
        parse_cmd( in );
    } catch( std::exception &e ) {
        std::cerr << e.what() << std::endl;
    }
    read_cli_cmd();
}

void CLI_Client::parse_cmd( const std::string &cmd ) {
    auto it = std::find_if(
        map.begin(),
        map.end(),
        [ &cmd ]( const std::pair<std::string,CONTENT> &v ) -> bool {
            return v.first.find( cmd ) == 0;
        }
    );
    if( it == map.end() ) {
        std::cout << "Invalid command" << std::endl;
        return;
    }
    Message outMsg;
    outMsg.type = TYPE::REQ;
    outMsg.cont = it->second;
    switch( it->second ) {
    case CONTENT::SHOW_NEI: {
        auto args = cmd.substr( it->first.size() );
        auto req = cmd_parse<Show_Neighbour_Req>( args );
        outMsg.data = serialize( req );
        break;
    }
    case CONTENT::SHOW_VER: break;
    case CONTENT::SHOW_TABLE: {
        auto args = cmd.substr( it->first.size() );
        auto req = cmd_parse<Show_Table_Req>( args );
        outMsg.data = serialize( req );
        break;
    }
    }
    auto outData = serialize( outMsg );
    sock.send( boost::asio::buffer( outData ) );
    auto len = sock.receive( boost::asio::buffer( buf ) );
    std::string inData { buf.data(), buf.data() + len };
    auto inMsg = deserialize<Message>( inData );
    if( inMsg.type != TYPE::RESP ) {
        std::cout << "Invalid type in response message" << std::endl;
        return;
    }
    switch( inMsg.cont ) {
    case CONTENT::SHOW_NEI: {
        auto resp = deserialize<Show_Neighbour_Resp>( inMsg.data );
        std::cout << resp << std::endl;
        break;
    }
    case CONTENT::SHOW_VER: break;
    case CONTENT::SHOW_TABLE: {
        auto resp = deserialize<Show_Table_Resp>( inMsg.data );
        std::cout << resp << std::endl;
        break;
    }
    }
}

template<>
Show_Table_Req cmd_parse<Show_Table_Req>( const std::string &args ) {
    return {};
}

template<>
Show_Neighbour_Req cmd_parse<Show_Neighbour_Req>( const std::string &args ) {
    return {};
}