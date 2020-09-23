#include <iomanip>
#include <chrono>
#include <ctime>

#include "log.hpp"

Logger::Logger( std::ostream &o ):
    os( o ),
    minimum( LOGL::INFO ),
    noop( false )
{}

void Logger::setLevel( const LOGL &level ) {
    minimum = level;
}

Logger& Logger::printTime() {
    auto in_time_t = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    *this << std::put_time( std::localtime( &in_time_t ), "%Y-%m-%d %X: ");
    return *this;
}

Logger& Logger::operator<<( std::ostream& (*fun)( std::ostream& ) ) {
    if( !noop ) {
        os << std::endl;
    }
    noop = false;
    return *this;
}

Logger& Logger::logInfo() {
    if( minimum > LOGL::INFO ) {
        noop = true;
    }
    return printTime();
}

Logger& Logger::logDebug() {
    if( minimum > LOGL::DEBUG ) {
        noop = true;
    }
    return printTime();
}

Logger& Logger::logError() {
    if( minimum > LOGL::ERROR ) {
        noop = true;
    }
    return printTime();
}

Logger& Logger::logAlert() {
    if( minimum > LOGL::ALERT ) {
        noop = true;
    }
    return printTime();
}