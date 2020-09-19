#ifndef VPP_API_HPP_
#define VPP_API_HPP_

#include "vapi/vapi.hpp"
#include "vapi/vpe.api.vapi.hpp"

#include "vapi/session.api.vapi.hpp"

struct vpp_api {
    vapi::Connection con;
    vpp_api();
    ~vpp_api();

};

#endif