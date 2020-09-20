#include "vpp.hpp"
#include "log.hpp"
#include "string_utils.hpp"

extern Logger logger;

DEFINE_VAPI_MSG_IDS_VPE_API_JSON
DEFINE_VAPI_MSG_IDS_SESSION_API_JSON

vpp_api::vpp_api() {
    auto ret = con.connect( "bgp++", nullptr, 32, 32 );
    if( ret == VAPI_OK ) {
        logger.logInfo() << LOGS::VPP << "VPP API: connected" << std::endl;
    } else {
        logger.logError() << LOGS::VPP << "VPP API: Cannot connect to vpp" << std::endl;
    }
}

vpp_api::~vpp_api() {
    auto ret = con.disconnect();
    if( ret == VAPI_OK ) {
        logger.logInfo() << LOGS::VPP << "VPP API: disconnected" << std::endl;
    } else {
        logger.logError() << LOGS::VPP << "VPP API: something went wrong, cannot disconnect" << std::endl;
    }
}