#pragma once
#include "vendor.h"

class coroutine_handler;
class util {
public:
    static const char* system_time();
    static void co_sleep(boost::asio::io_context& io, std::chrono::milliseconds ms, coroutine_handler& ch);
    static const char* random_string(int size);
};
