#include <iostream>
#include <fstream>
#include <boost/asio/ip/network_v4.hpp>
#include <yaml-cpp/yaml.h>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "main.hpp"
#include "config.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "yaml.hpp"
#include "evloop.hpp"
#include "packet.hpp"

Logger logger;

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
        YAML::Node config = YAML::LoadFile( "config.yaml" );
        conf = config.as<GlobalConf>();
    } catch( std::exception &e ) {
        logger.logError() << LOGS::MAIN << "Cannot load config: " << e.what() << std::endl;
        return 1;
    }

    logger.logInfo() << LOGS::MAIN << "Loaded conf: " << std::endl;
    logger.logInfo() << LOGS::MAIN << conf;

    try { 
        EVLoop loop { conf };
        loop.run();
    } catch( std::exception &e ) {
        logger.logError() << LOGS::MAIN << "Error on run event loop: " << e.what() << std::endl;
    }
    return 0;
}