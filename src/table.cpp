#include <boost/asio/ip/network_v4.hpp>
#include <map>
#include <chrono>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;

#include "table.hpp"
#include "fsm.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "log.hpp"
#include "string_utils.hpp"

using prefix_v4 = boost::asio::ip::network_v4;

extern Logger logger;

bgp_path::bgp_path( std::shared_ptr<std::vector<path_attr_t>> a, std::shared_ptr<bgp_fsm> s ):
    attrs( std::move( a ) ),
    source( std::move( s ) ),
    time( std::chrono::system_clock::now() ),
    isValid( true ),
    isBest( false )
{}

uint32_t bgp_path::get_local_pref() const {
    for( auto const &el: *attrs ) {
        if( el.type == PATH_ATTRIBUTE::LOCAL_PREF ) {
            return el.get_u32();
        }
    }
    throw std::runtime_error( "No such attribute" );
}

uint32_t bgp_path::get_med() const {
    for( auto const &el: *attrs ) {
        if( el.type == PATH_ATTRIBUTE::MULTI_EXIT_DISC ) {
            return el.get_u32();
        }
    }
    throw std::runtime_error( "No such attribute" );
}

ORIGIN bgp_path::get_origin() const {
    for( auto const &el: *attrs ) {
        if( el.type == PATH_ATTRIBUTE::ORIGIN ) {
            return static_cast<ORIGIN>( el.get_u32() );
        }
    }
    throw std::runtime_error( "No such attribute" );
}

std::vector<uint32_t> bgp_path::get_as_path() const {
    for( auto const &el: *attrs ) {
        if( el.type == PATH_ATTRIBUTE::LOCAL_PREF ) {
            return el.parse_as_path();
        }
    }
    throw std::runtime_error( "No such attribute" );
}


bgp_table_v4::bgp_table_v4( GlobalConf &c ):
    conf( c )
{}

void bgp_table_v4::add_path( prefix_v4 prefix, std::vector<path_attr_t> attr, std::shared_ptr<bgp_fsm> nei ) {
    // Add local preference attribute, if it doesn't exist
    if(
        auto it = std::find_if(
            attr.begin(),
            attr.end(),
            []( const path_attr_t &v ) {
                return v.type == PATH_ATTRIBUTE::LOCAL_PREF;
            }
        ); it == attr.end() )
    {
        path_attr_t lp;
        uint32_t def_lp = bswap( (uint32_t)100 );
        lp.type = PATH_ATTRIBUTE::LOCAL_PREF;
        lp.bytes.resize( sizeof( def_lp ) );
        std::memcpy( lp.bytes.data(), &def_lp, sizeof( def_lp ) );
        attr.push_back( std::move( lp ) );
    }
    // If we already have path from this neighbour
    for( auto prefixIt = table.find( prefix ); prefixIt != table.end(); prefixIt++ ) {
        if( prefixIt->second.source != nei ) {
            continue;
        }
        prefixIt->second.time = std::chrono::system_clock::now();
        prefixIt->second.attrs.reset();
        prefixIt->second.attrs = std::make_shared<std::vector<path_attr_t>>( std::move( attr ) );
        return;
    }
    // If not, we will look if path already exists
    auto pathIt = std::find_if( 
        table.begin(), 
        table.end(), 
        [ &attr ]( const std::pair<prefix_v4,bgp_path> &pair ) -> bool {
            return *pair.second.attrs == attr;
        }
    );
    if( pathIt == table.end() ) {
        table.emplace( std::piecewise_construct,
            std::forward_as_tuple( prefix ), 
            std::forward_as_tuple( std::make_shared<std::vector<path_attr_t>>( std::move( attr ) ), nei )
        );
    } else {
        table.emplace( std::piecewise_construct, 
            std::forward_as_tuple( prefix ), 
            std::forward_as_tuple( pathIt->second )
        );
    }
}

void bgp_table_v4::del_path( prefix_v4 prefix, std::shared_ptr<bgp_fsm> nei ) {
    for( auto prefixIt = table.find( prefix ); prefixIt != table.end(); prefixIt++ ) {
        if( prefixIt->second.source != nei ) {
            continue;
        }
        table.erase( prefixIt );
        return;
    }
}

void bgp_table_v4::best_path_selection() {
    prefix_v4 current_prefix;
    std::multimap<prefix_v4,bgp_path>::iterator best = table.end();

    for( auto currentIt = table.begin(); currentIt != table.end(); currentIt++ ) {
        auto const &prefix = currentIt->first;
        auto &path = currentIt->second;

        path.isValid = true;

        if( current_prefix != prefix ) {
            current_prefix = prefix;
            best->second.isBest = true;
            best = table.end();
        }

        if( best == table.end() ) {
            best = currentIt;
            path.isBest = true;
        }

        if( best != currentIt ) {
            path.isBest = false;
        }

        if( best != currentIt ) {
            try {
                if( currentIt->second.get_local_pref() > best->second.get_local_pref() ) {
                    best = currentIt; continue;
                }
            } catch( std::exception &e ) {
                logger.logError() << LOGS::TABLE << e.what() << std::endl;
            }

            try {
                if( currentIt->second.get_as_path().size() < best->second.get_as_path().size() ) {
                    best = currentIt; continue;
                }
            } catch( std::exception &e ) {
                logger.logError() << LOGS::TABLE << e.what() << std::endl;
            }

            try {
                if( currentIt->second.get_origin() < best->second.get_origin() ) {
                    best = currentIt; continue;
                }
            } catch( std::exception &e ) {
                logger.logError() << LOGS::TABLE << e.what() << std::endl;
            }

            try {
                if( currentIt->second.get_med() > best->second.get_med() ) {
                    best = currentIt; continue;
                }
            } catch( std::exception &e ) {
                logger.logError() << LOGS::TABLE << e.what() << std::endl;
            }
            // TODO: other BPS conditionds
        }
    }

    if( best == table.end() ) {
        best->second.isBest = true;
    }
}