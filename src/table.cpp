#include <boost/asio/ip/network_v4.hpp>
#include <map>
#include <chrono>

using address_v4 = boost::asio::ip::address_v4;
using prefix_v4 = boost::asio::ip::network_v4;
using nlri = prefix_v4;

#include "table.hpp"
#include "fsm.hpp"
#include "packet.hpp"
#include "config.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "evloop.hpp"

extern Logger logger;
extern std::shared_ptr<EVLoop> runtime;

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


bgp_table_v4::bgp_table_v4( boost::asio::io_context &i, GlobalConf &c ):
    io( i ),
    conf( c ),
    send_updates( io )
{
    for( auto &r: conf.originate_routes ) {
        std::vector<path_attr_t> attrs;

        path_attr_t attr;
        attr.make_local_pref( 100 );
        attrs.push_back( attr );

        attr.make_origin( ORIGIN::IGP );
        attrs.push_back( attr );

        attr.make_nexthop( boost::asio::ip::make_address_v4( "0.0.0.0" ) );
        attrs.push_back( attr );

        // TODO: route policy
        add_path( r.prefix, attrs, nullptr );
    }
}

void bgp_table_v4::add_path( const prefix_v4 &prefix, std::vector<path_attr_t> attr, std::shared_ptr<bgp_fsm> nei ) {
    scheduled_updates.emplace( prefix );
    schedule_updates();
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
        lp.make_local_pref( 100 );
        attr.push_back( std::move( lp ) );
    }
    // If we already have path from this neighbour
    auto range = table.equal_range( prefix );
    for( auto prefixIt = range.first; prefixIt != range.second; prefixIt++ ) {
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

void bgp_table_v4::del_path( const prefix_v4 &prefix, std::shared_ptr<bgp_fsm> nei ) {
    scheduled_updates.emplace( prefix );
    schedule_updates();
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

void bgp_table_v4::purge_peer( std::shared_ptr<bgp_fsm> peer ) {
    for( auto it = table.begin(); it != table.end(); ) {
        if( it->second.source == peer ) {
            scheduled_updates.emplace( it->first );
            it = table.erase( it );
        } else {
            it++;
        }
    }
    schedule_updates();
}

void bgp_table_v4::schedule_updates() {
    send_updates.expires_from_now( std::chrono::seconds( 1 ) );
    send_updates.async_wait( std::bind( &bgp_table_v4::on_send_updates, this, std::placeholders::_1 ) );
}

void bgp_table_v4::on_send_updates( const boost::system::error_code &ec ) {
    if( ec ) {
        logger.logError() << LOGS::EVENT_LOOP << "On timer for sending updates: " << ec.message() << std::endl;
    }
    std::vector<nlri> withdrawn_update;
    std::map<std::shared_ptr<std::vector<path_attr_t>>,std::vector<nlri>> pending_update;
    for( auto const &n: scheduled_updates ) {
        auto it = table.end();
        if( it = table.find( n ); it == table.end() ) {
            withdrawn_update.push_back( n );
            continue;
        }
        if( auto updIt = pending_update.find( it->second.attrs ); updIt != pending_update.end() ) {
            updIt->second.push_back( n );
        } else {
            std::vector<nlri> new_vec { n };
            pending_update.emplace( it->second.attrs, new_vec );
        }
    }

    for( auto const &[ add, nei ]: runtime->neighbours ) {
        // send updates only for eBGP neighbours
        if( nei->conf.remote_as == conf.my_as ) {
            continue;
        }

        if( nei->state != FSM_STATE::ESTABLISHED ) {
            continue;
        }

        auto cur_withdrawn = withdrawn_update;
        for( auto const &[ path, n_vec ]: pending_update ) {
            nei->tx_update( n_vec, path, cur_withdrawn );
            cur_withdrawn.clear();
        }
    }
}