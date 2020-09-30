#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>

enum class LOGL: uint8_t {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    ALERT
};

enum class LOGS: uint8_t {
    MAIN,
    PACKET,
    BGP,
    EVENT_LOOP,
    FSM,
    CONNECTION,
    CONFIGURATION,
    VPP,
    CLI
};

class Logger {
private:
    std::ostream &os;
    LOGL minimum;
    bool noop;
    Logger& printTime();

public:
    Logger( std::ostream &o = std::cout );

    Logger& logInfo();
    Logger& logDebug();
    Logger& logError();
    Logger& logAlert();
    void setLevel( const LOGL &level );
    Logger& operator<<( std::ostream& (*fun)( std::ostream& ) );

    template<typename T>
    Logger& operator<<( const T& data ) {
        if( !noop ) {
            os << data;
        }
        return *this;
    }
};

#endif