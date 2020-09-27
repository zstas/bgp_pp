#ifndef MAIN_HPP
#define MAIN_HPP

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