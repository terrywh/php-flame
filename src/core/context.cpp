#include "context.hpp"
#include "coroutine.hpp"

namespace flame { namespace core {

    std::unique_ptr<context> $ { new context() };

    context::context()
    : env(boost::this_process::environment())
    , opt(new php::value(php::array::create(4)))  {

    }

    // 协程：
    void context::co_sleep(std::chrono::milliseconds ms, coroutine_handler& ch, boost::asio::io_context& io) {
        boost::asio::steady_timer tm(io);
        tm.expires_after(ms);
        tm.async_wait(ch);
    }
}}
