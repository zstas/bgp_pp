#ifndef NLRI_HPP
#define NLRI_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <iosfwd>

enum class BGP_AFI : uint16_t;

class NLRI {
public:
    explicit NLRI( BGP_AFI, uint8_t *p );
    explicit NLRI( BGP_AFI, const std::string &prefix );

    std::vector<uint8_t> serialize() const;

    friend std::ostream& operator<<( std::ostream &os, const NLRI &n );
private:
    BGP_AFI afi;
    std::vector<uint8_t> data;
    uint16_t nlri_len;
};

std::ostream& operator<<( std::ostream &os, const NLRI &n );

#endif