#ifndef CLI_HPP
#define CLI_HPP

class EVLoop;

class CLI_Session: public std::enable_shared_from_this<CLI_Session> {
public:
    CLI_Session( boost::asio::io_context &i, boost::asio::local::stream_protocol::socket s, std::shared_ptr<EVLoop> r );
    void start();
private:
    void on_receive( const boost::system::error_code &ec, std::size_t len );

    std::array<char,2048> buf;
    boost::asio::io_context &io;
    boost::asio::local::stream_protocol::socket sock;
    std::shared_ptr<EVLoop> &runtime;
};

class CLI_Server : public std::enable_shared_from_this<CLI_Server> {
public:
    CLI_Server( boost::asio::io_context &io, const std::string &path, std::shared_ptr<EVLoop> r );
    void start();
private:
    void on_accept( const boost::system::error_code &ec );
    boost::asio::io_context &io;
    boost::asio::local::stream_protocol::endpoint ep;
    boost::asio::local::stream_protocol::acceptor acceptor;
    boost::asio::local::stream_protocol::socket sock;
    std::shared_ptr<EVLoop> &runtime;
};

#endif