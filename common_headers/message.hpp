#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <chrono>
#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

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
    uint32_t local_pref { 0U };
    std::string source;
    std::string time;
    std::string as_path;
    bool best { false };

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & prefix;
        archive & nexthop;
        archive & local_pref;
        archive & source;
        archive & time;
        archive & as_path;
        archive & best;
    }
};

struct Show_Table_Resp {
    std::vector<BGP_Entry> entries;

    template<class Archive>
    void serialize( Archive &archive, const unsigned int version ) {
        archive & entries;
    }
};

static auto const ser_flags = boost::archive::no_header | boost::archive::no_tracking;

template<typename T>
std::string serialize( const T &val ) {
    std::stringstream ss;
    boost::archive::binary_oarchive ser( ss, ser_flags );
    ser << val;
    return ss.str();
}

template<typename T>
T deserialize( const std::string &val ) {
    T out;
    std::istringstream ss( val );
    boost::archive::binary_iarchive deser{ ss, ser_flags };
    deser >> out;
    return out;
}

#endif