#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <chrono>
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

struct Show_Table_Req {
    boost::optional<std::string> prefix;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & prefix;
    }
};

struct BGP_Entry {
    std::string prefix;
    std::string nexthop;
    uint32_t local_pref;
    std::string source;
    std::chrono::system_clock::time_point time;
    std::string as_path;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & prefix;
        archive & nexthop;
        archive & local_pref;
        archive & source;
        archive & time;
        archive & as_path;
    }
};

struct Show_Table_Resp {
    std::vector<BGP_Entry> entries;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & entries;
    }
};

#endif