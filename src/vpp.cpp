#include "main.hpp"

DEFINE_VAPI_MSG_IDS_VPE_API_JSON
DEFINE_VAPI_MSG_IDS_SESSION_API_JSON

vpp_api::vpp_api() {
    log( "vpp_api cstr" );
    auto ret = con.connect( "bgp++", nullptr, 32, 32 );
    if( ret == VAPI_OK ) {
        log("VPP API: connected");
    } else {
        log( "VPP API: Cannot connect to vpp" );
    }
}

vpp_api::~vpp_api() {
    auto ret = con.disconnect();
    if( ret == VAPI_OK ) {
        log("VPP API: disconnected");
    } else {
        log("VPP API: something went wrong, cannot disconnect");
    }
}

