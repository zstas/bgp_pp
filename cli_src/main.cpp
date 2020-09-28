#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "main.hpp"
#include "string_utils.hpp"
#include "message.hpp"
#include "clicl.hpp"

bool interrupted { false };

void sighandler( boost::system::error_code ec, int signal ) {
    interrupted = true;
}

int main(int argc, char *argv[])
{
    std::string unix_socket_path { "/var/run/bgp++.sock" };
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ( "help,h", "print this useful message" )
        ( "version,v", "print version" )
        ( "path,p", boost::program_options::value<std::string>( &unix_socket_path ), "path to unix socket for cli access" )
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

    boost::asio::io_context io;
    CLI_Client cli { io, unix_socket_path };
    boost::asio::signal_set set { io, SIGTERM, SIGINT, SIGKILL };
    set.async_wait( sighandler );

    while( !interrupted ) {
        io.run();
    }

    std::string binary_data;

    {
        Show_Table_Req st;
        st.prefix.emplace( "10.0.0.0/24" );
        Message msg;
        msg.cont = CONTENT::SHOW_TABLE;
        msg.type = TYPE::REQ;
        auto inner = serialize( st );
        msg.data = inner;
        binary_data = serialize( msg );
    }

    {
        auto msg = deserialize<Message>( binary_data );
        std::cout << msg << std::endl;
        switch( msg.cont ) {
        case CONTENT::SHOW_NEI: break;
        case CONTENT::SHOW_VER: break;
        case CONTENT::SHOW_TABLE: {
            auto st = deserialize<Show_Table_Req>( msg.data );
            std::cout << st << std::endl;
            break;
        }
        }
    }

    return 0;
}