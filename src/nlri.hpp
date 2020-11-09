#ifndef NLRI_HPP
#define NLRI_HPP

#include <cstdint>
#include <vector>
#include <string>

enum class BGP_AFI : uint16_t;

class NLRI {
public:
    explicit NLRI( uint8_t *p, uint16_t len );
    explicit NLRI( BGP_AFI, const std::string &prefix );

    std::vector<uint8_t> serialize() const;
    std::string as_ipv4() const;
    std::string as_ipv6() const;
private:
    std::vector<uint8_t> data;
    uint16_t nlri_len;
};

#endif