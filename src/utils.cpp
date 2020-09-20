#include <stdint.h>
#include "utils.hpp"

uint16_t bswap16( uint16_t value ) noexcept {
    return __builtin_bswap16( value );
}

uint32_t bswap32( uint32_t value ) noexcept {
    return __builtin_bswap32( value );
}
