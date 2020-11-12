#include <iostream>
#include <fstream>
#include <boost/asio/ip/address_v4.hpp>
#include <yaml-cpp/yaml.h>
#include <boost/program_options.hpp>

using address_v4 = boost::asio::ip::address_v4;

#include "main.hpp"
#include "config.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "yaml.hpp"
#include "evloop.hpp"
#include "packet.hpp"
#include "cli.hpp"
#include "nlri.hpp"

Logger logger;
std::shared_ptr<EVLoop> runtime;

static void config_init( const std::string &path ) {
    GlobalConf new_conf;
    new_conf.listen_on_port = 179;
    new_conf.my_as = 31337;
    new_conf.hold_time = 60;
    new_conf.bgp_router_id = address_v4::from_string( "1.2.3.4" );

    bgp_neighbour_v4 bgp1;
    bgp1.remote_as = 31337;
    bgp1.address = address_v4::from_string( "127.0.0.1" );
    new_conf.neighbours.emplace_back( bgp1 );

    bgp1.address = address_v4::from_string( "1.1.1.1" );
    new_conf.neighbours.emplace_back( bgp1 );

    OrigEntry orig;
    orig.prefix = NLRI( BGP_AFI::IPv4, "1.2.3.4/32" );
    new_conf.originate_routes.push_back( orig );

    orig.prefix = NLRI( BGP_AFI::IPv4, "6.6.6.6/32" );
    orig.policy_name.emplace( "route_policy1.yaml" );
    new_conf.originate_routes.push_back( orig );

    YAML::Node node;
    node = new_conf;
    std::ofstream fout( path );
    fout << node << std::endl;
}

int main( int argc, char *argv[] ) {
    std::string unix_socket_path { "/var/run/bgp++.sock" };
    std::string config_path { "config.yaml" };

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ( "gen_conf,g", "generate basic configuration file" )
        ( "help,h", "print this useful message" )
        ( "version,v", "print version" )
        ( "path,p", boost::program_options::value<std::string>( &unix_socket_path ), "path to unix socket for cli access" )
        ( "config,c", boost::program_options::value<std::string>( &config_path ), "path to configuration file" )
    ;

    boost::program_options::positional_options_description p;
    boost::program_options::variables_map vm;
    boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( desc ).positional( p ).run(), vm );

    if( vm.count( "help" ) ) {
        std::cout << desc << std::endl;
    }

    if( vm.count( "version" ) ) {
        std::cout << "Version: 1.2.3" << std::endl;
    }

    if( vm.count( "gen_conf" ) ) {
        config_init( config_path );    
    }

    GlobalConf conf;
    try {
        YAML::Node config = YAML::LoadFile( config_path );
        conf = config.as<GlobalConf>();
    } catch( std::exception &e ) {
        logger.logError() << LOGS::MAIN << "Cannot load config: " << e.what() << std::endl;
        return 1;
    }

    logger.logInfo() << LOGS::MAIN << "Loaded conf: " << std::endl;
    logger.logInfo() << LOGS::MAIN << conf;

    unlink( unix_socket_path.c_str() );

    boost::asio::io_context io;
    runtime = std::make_shared<EVLoop>( io, conf );
    auto cli = std::make_shared<CLI_Server>( io, unix_socket_path, runtime );
    cli->start();
    runtime->start();
    while( true ) {
        try { 
            io.run();
        } catch( std::exception &e ) {
            logger.logError() << LOGS::MAIN << "Error on run event loop: " << e.what() << std::endl;
        }
    }
    return 0;
}