#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <iosfwd>

enum class TYPE: uint8_t;
enum class CONTENT: uint8_t;
struct Message;
struct Show_Table_Req;
struct Show_Table_Resp;
struct Show_Neighbour_Resp;

std::ostream& operator<<( std::ostream &os, const std::vector<uint8_t> &data );
std::ostream& operator<<( std::ostream &os, const TYPE &typ );
std::ostream& operator<<( std::ostream &os, const CONTENT &cont );
std::ostream& operator<<( std::ostream &os, const Message &msg );
std::ostream& operator<<( std::ostream &os, const Show_Table_Req &msg );
std::ostream& operator<<( std::ostream &os, const Show_Table_Resp &msg );
std::ostream& operator<<( std::ostream &os, const Show_Neighbour_Resp &msg );

#endif