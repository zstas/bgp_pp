#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "clicl.hpp"

CLI_Client::CLI_Client( boost::asio::io_context &i, const std::string &path ):
    io( i ),
    ep( path ),
    sock( io, ep ),
    input( io, STDIN_FILENO )
{
    sock.async_connect( ep, std::bind( &CLI_Client::on_connect, this, std::placeholders::_1 ) );
}

void CLI_Client::on_connect( const boost::system::error_code &ec ) {
    if( ec ) {
        std::cerr << ec.message() << std::endl;
        sleep( 3 );
        sock.async_connect( ep, std::bind( &CLI_Client::on_connect, this, std::placeholders::_1 ) );
    }
}

std::string CLI_Client::sync_send( std::string out ) {
    sock.send( boost::asio::buffer( out ) );
    auto len = sock.receive( boost::asio::buffer( buf ) );
    return { buf.data(), buf.data() + len };
}