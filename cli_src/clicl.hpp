#ifndef CLICL_HPP
#define CLICL_HPP

#include <list>
#include <functional>

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

    std::array<uint8_t,2048> buf;
    boost::asio::io_context &io;
    boost::asio::local::stream_protocol::endpoint ep;
    boost::asio::local::stream_protocol::socket sock;
    boost::asio::posix::stream_descriptor input;
};

#endif