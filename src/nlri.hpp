#ifndef NLRI_HPP
#define NLRI_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <iosfwd>

enum class BGP_AFI : uint16_t;

class NLRI {
public:
    NLRI() = default;
    explicit NLRI( BGP_AFI, uint8_t *p, uint8_t len );
    explicit NLRI( BGP_AFI, const std::string &prefix );

    std::vector<uint8_t> serialize() const;
    std::string to_string() const;

    friend std::ostream& operator<<( std::ostream &os, const NLRI &n );
    friend bool operator<( const NLRI &lhv,const NLRI &rhv );
    friend bool operator==( const NLRI &lhv,const NLRI &rhv );
    friend bool operator!=( const NLRI &lhv,const NLRI &rhv );
private:
    BGP_AFI afi;
    std::vector<uint8_t> data;
    uint8_t nlri_len;
};

std::ostream& operator<<( std::ostream &os, const NLRI &n );
bool operator<( const NLRI &lhv,const NLRI &rhv );
bool operator==( const NLRI &lhv,const NLRI &rhv );
bool operator!=( const NLRI &lhv,const NLRI &rhv );

#endif