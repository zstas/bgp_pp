#include <iostream>
#include <fstream>
#include <boost/asio/ip/network_v4.hpp>
#include <yaml-cpp/yaml.h>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "main.hpp"
#include "fsm.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "yaml.hpp"
#include "packet.hpp"

Logger logger;

main_loop::main_loop( GlobalConf &c ):
    conf( c ),
    accpt( io, endpoint( boost::asio::ip::tcp::v4(), c.listen_on_port ) ),
    sock( io )
{
    for( auto &nei: c.neighbours ) {
        neighbours.emplace( nei.address, std::make_shared<bgp_fsm>( io, c, nei ) );
    }
}

void main_loop::run() {
    logger.logInfo() << LOGS::EVENT_LOOP << "Starting event loop" << std::endl;
    accpt.async_accept( sock, std::bind( &main_loop::on_accept, this, std::placeholders::_1 ) );
    io.run();
}

void main_loop::on_accept( error_code ec ) {
    if( ec ) {
        logger.logError() << LOGS::EVENT_LOOP << "Error on accepting new connection: " << ec.message() << std::endl;
    }
    auto const &remote_addr = sock.remote_endpoint().address().to_v4();
    auto const &nei_it = neighbours.find( remote_addr );
    if( nei_it == neighbours.end() ) {
        logger.logInfo() << LOGS::EVENT_LOOP << "Connection not from our peers, so dropping it." << std::endl;
        sock.close();
    } else {
        nei_it->second->place_connection( std::move( sock ) );
    }
    accpt.async_accept( sock, std::bind( &main_loop::on_accept, this, std::placeholders::_1 ) );
}

static void config_init() {
    GlobalConf new_conf;
    new_conf.listen_on_port = 179;
    new_conf.my_as = 31337;
    new_conf.hold_time = 60;
    new_conf.bgp_router_id = address_v4::from_string( "1.2.3.4" );

    bgp_neighbour_v4 bgp1;
    bgp1.remote_as = 31337;
    bgp1.address = address_v4::from_string( "127.0.0.1" );
    new_conf.neighbours.emplace_back( bgp1 );

    bgp1.address = address_v4::from_string( "8.8.8.8" );
    new_conf.neighbours.emplace_back( bgp1 );

    YAML::Node node;
    node = new_conf;
    std::ofstream fout("config.yaml");
    fout << node << std::endl;
}

int main( int argc, char *argv[] ) {
    config_init();    
    GlobalConf conf;
    try {
        YAML::Node config = YAML::LoadFile("config.yaml");
        conf = config.as<GlobalConf>();
    } catch( std::exception &e ) {
        logger.logError() << LOGS::MAIN << "Cannot load config: " << e.what() << std::endl;
        return 1;
    }

    logger.logInfo() << "Loaded conf: " << std::endl;
    logger.logInfo() << conf << std::endl;

    logger.logInfo() << "Binding port in vpp" << std::endl;
    try { 
        main_loop loop { conf };
        loop.run();
    } catch( std::exception &e ) {
        logger.logError() << LOGS::MAIN << "Error on run event loop: " << e.what() << std::endl;
    }
    return 0;
}