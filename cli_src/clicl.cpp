#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "clicl.hpp"

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
    std::cout << "bgp# " << std::endl;
    boost::system::error_code ec;
    auto len = input.read_some( boost::asio::buffer( buf ), ec );
    if( ec ) {
        std::cerr << ec.message() << std::endl;
        exit( -1 );
    }
}