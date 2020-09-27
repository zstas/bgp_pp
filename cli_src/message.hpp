#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <boost/optional.hpp>

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
    std::string data;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & type;
        archive & cont;
        archive & data;
    }
};

struct Show_Table {
    boost::optional<std::string> prefix;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & prefix;
    }
};

#endif