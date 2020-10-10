#ifndef CLICL_HPP
#define CLICL_HPP

#include <list>
#include <functional>

enum class CONTENT: uint8_t;
struct Show_Table_Req;
struct Show_Neighbour_Req;

enum class TOKEN: uint8_t {
    CONT,
    ARG,
    END
};

struct CLI_Token {
    TOKEN tok;
    std::string data;
    std::list<CLI_Token> next_attrs;
    std::function<void(const std::map<std::string,std::string>)> callback;

    void lookup( std::list<std::string> &cmd, std::map<std::string,std::string> &args );
};

class CLI_Client {
public:
    CLI_Client( boost::asio::io_context &i, const std::string &path );
    std::string sync_send( std::string out );
private:
    void on_connect( const boost::system::error_code &ec );
    void read_cli_cmd();
    void parse_cmd( const std::string &cmd );

    std::map<std::string,CONTENT> map;
    std::array<uint8_t,2048> buf;
    boost::asio::io_context &io;
    boost::asio::local::stream_protocol::endpoint ep;
    boost::asio::local::stream_protocol::socket sock;
    boost::asio::posix::stream_descriptor input;
};

template<typename T>
T cmd_parse( const std::string &args );

template<>
Show_Table_Req cmd_parse<Show_Table_Req>( const std::string &args );

template<>
Show_Neighbour_Req cmd_parse<Show_Neighbour_Req>( const std::string &args );

#endif