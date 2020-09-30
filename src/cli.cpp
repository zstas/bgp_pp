#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include "cli.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "message.hpp"

extern Logger logger;

CLI_Session::CLI_Session( boost::asio::io_context &i, boost::asio::local::stream_protocol::socket s ):
    io( i ),
    sock( std::move( s ) )
{}

void CLI_Session::start() {
    sock.async_receive( boost::asio::buffer( buf ), std::bind( &CLI_Session::on_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2 ) );
}

void CLI_Session::on_receive( const boost::system::error_code &ec, std::size_t len ) {
    if( ec ) {
        logger.logError() << LOGS::CLI << ec.message() << std::endl;
        start();
        return;
    }
    std::string inMsg { buf.data(), buf.data() + len };
    
    start();
}

CLI_Server::CLI_Server( boost::asio::io_context &i, const std::string &path ):
    io( i ),
    ep( path ),
    acceptor( io, ep ),
    sock( io )
{}

void CLI_Server::start() {
    acceptor.async_accept( sock, std::bind( &CLI_Server::on_accept, shared_from_this(), std::placeholders::_1 ) );
}

void CLI_Server::on_accept( const boost::system::error_code &ec ) {
    if( ec ) {
        logger.logError() << LOGS::CLI << ec.message() << std::endl;
        start();
        return;
    }
    logger.logInfo() << LOGS::CLI << "Accepted new CLI session" << std::endl;
    auto session = std::make_shared<CLI_Session>( io, std::move( sock ) );
    session->start();
    start();
}