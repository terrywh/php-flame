#pragma once
#include "vendor.h"

class ipc_abstract {
public:
    
    boost::asio::local::stream_protocol::socket    socket_;
    boost::asio::local::stream_protocol::endpoint address_;
};
