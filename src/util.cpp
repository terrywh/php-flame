#include "util.h"
#include "coroutine.h"

const char* util::system_time() {
    static char buffer[24] = {0};
    std::time_t t = std::time(nullptr);
    struct tm *m = std::localtime(&t);
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            1900 + m->tm_year,
            1 + m->tm_mon,
            m->tm_mday,
            m->tm_hour,
            m->tm_min,
            m->tm_sec);
    return buffer;
}


void util::co_sleep(boost::asio::io_context& io, std::chrono::milliseconds ms, coroutine_handler& ch) {
    boost::asio::steady_timer tm(io);
    tm.expires_after(ms);
    tm.async_wait(ch);
}