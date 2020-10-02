#include <iostream>
#include <memory>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/ip/network_v4.hpp>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "cli.hpp"
#include "evloop.hpp"
#include "fsm.hpp"
#include "packet.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "message.hpp"

extern Logger logger;

CLI_Session::CLI_Session( boost::asio::io_context &i, boost::asio::local::stream_protocol::socket s, EVLoop &r ):
    io( i ),
    sock( std::move( s ) ),
    runtime( r )
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
    std::string inData { buf.data(), buf.data() + len };
    auto inMsg = deserialize<Message>( inData );
    if( inMsg.type != TYPE::REQ ) {
        logger.logError() << LOGS::CLI << "This is not a request, so dropping it." << std::endl;
        start();
        return;
    }

    logger.logInfo() << LOGS::CLI << "Got new request from cli session: " << inMsg.cont << std::endl;

    Message outMsg;
    outMsg.type = TYPE::RESP;
    outMsg.cont = inMsg.cont;

    switch( inMsg.cont ) {
    case CONTENT::SHOW_NEI: break;
    case CONTENT::SHOW_TABLE: {
        auto req = deserialize<Show_Table_Req>( inMsg.data );
        // TODO: handle req
        Show_Table_Resp resp;
        for( auto const &[ add, nei ]: runtime.neighbours ) {
            for( auto const &[ prefix, path ]: nei->table.table ) {
                BGP_Entry entry;
                auto in_time_t = std::chrono::system_clock::to_time_t( path->time );
                std::stringstream stream;
                stream << std::put_time( std::localtime( &in_time_t ), "%Y-%m-%d %X: ");
                entry.time = stream.str();
                entry.prefix = prefix.to_string();
                for( auto const &attr: path->attrs ) {
                    if( attr.type == PATH_ATTRIBUTE::NEXT_HOP ) {
                        entry.nexthop = boost::asio::ip::make_address_v4( attr.get_u32() ).to_string();
                    } else if( attr.type == PATH_ATTRIBUTE::LOCAL_PREF ) {
                        entry.local_pref = attr.get_u32();
                    }
                }
                resp.entries.push_back( entry );
            }
        }
        outMsg.data = serialize( resp );
        auto outData = serialize( outMsg );
        sock.send( boost::asio::buffer( outData ) );
        break;
    }
    case CONTENT::SHOW_VER: break;
    }
    start();
}

CLI_Server::CLI_Server( boost::asio::io_context &i, const std::string &path, EVLoop &r ):
    io( i ),
    ep( path ),
    acceptor( io, ep ),
    sock( io ),
    runtime( r )
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
    auto session = std::make_shared<CLI_Session>( io, std::move( sock ), runtime );
    session->start();
    start();
}