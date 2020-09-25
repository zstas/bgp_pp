#include <iostream>
#include <vector>
#include <optional>
#include <boost/program_options.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

enum class TYPE: uint8_t {
    REQ,
    RESP
};

enum class CONTENT: uint8_t {
    SHOW_VER,
    SHOW_TABLE,
    SHOW_NEI
};

struct Message {
    TYPE type;
    CONTENT cont;
    std::vector<uint8_t> data;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & type;
        archive & cont;
        archive & data;
    }
};

struct Show_Table {
    std::optional<std::string> prefix;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        if( prefix.has_value() ) {
            archive & prefix;
        }
    }
};

int main(int argc, char *argv[])
{
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ( "help,h", "print this useful message" )
        ( "version,v", "print version" )
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

    Show_Table st;
    st.prefix.emplace( "10.0.0.0/24" );
    Message msg;
    msg.cont = CONTENT::SHOW_TABLE;
    msg.type = TYPE::REQ;
    msg.data = { 1, 2, 3, 4 };

    boost::archive::text_oarchive ser( std::cout );
    ser << msg;

    return 0;
}