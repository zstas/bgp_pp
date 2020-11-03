#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <iosfwd>

enum class LOGL: uint8_t;
enum class LOGS: uint8_t;
enum class PATH_ATTRIBUTE : uint8_t;
enum class ORIGIN : uint8_t;
enum class BGP_CAP_CODE : uint8_t;
enum class BGP_ERR_CODE : uint8_t;
enum class BGP_MSG_HDR_ERR : uint8_t;
enum class BGP_OPEN_ERR : uint8_t;
enum class BGP_UPDATE_ERR : uint8_t;
enum class BGP_FSM_ERR : uint8_t;
enum class BGP_CEASE_ERR : uint8_t;
struct bgp_open;
struct path_attr_t;
struct bgp_cap_t;
struct GlobalConf;
struct bgp_neighbour_v4;
enum class CONTENT: uint8_t;

std::ostream& operator<<( std::ostream &os, const LOGL &l );
std::ostream& operator<<( std::ostream &os, const LOGS &l );
std::ostream& operator<<( std::ostream &os, const GlobalConf &conf );
std::ostream& operator<<( std::ostream &os, const std::vector<uint8_t> &val );
std::ostream& operator<<( std::ostream &os, const bgp_neighbour_v4 &nei );
std::ostream& operator<<( std::ostream &os, const bgp_open *open );
std::ostream& operator<<( std::ostream &os, const path_attr_t &attr );
std::ostream& operator<<( std::ostream &os, const bgp_cap_t &cont );
std::ostream& operator<<( std::ostream &os, const PATH_ATTRIBUTE &attr );
std::ostream& operator<<( std::ostream &os, const ORIGIN &orig );
std::ostream& operator<<( std::ostream &os, const CONTENT &cont );
std::ostream& operator<<( std::ostream &os, const BGP_CAP_CODE &cont );
std::ostream& operator<<( std::ostream &os, const BGP_ERR_CODE &err );
std::ostream& operator<<( std::ostream &os, const BGP_MSG_HDR_ERR &err );
std::ostream& operator<<( std::ostream &os, const BGP_OPEN_ERR &err );
std::ostream& operator<<( std::ostream &os, const BGP_UPDATE_ERR &err );
std::ostream& operator<<( std::ostream &os, const BGP_FSM_ERR &err );
std::ostream& operator<<( std::ostream &os, const BGP_CEASE_ERR &err );

#endif