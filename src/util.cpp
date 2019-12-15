#include "util.h"
#include "coroutine.h"
#include <random>

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

static const char random_chars[] = "AABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567899";
static std::default_random_engine random_genr((unsigned long)(&random_chars));
static std::uniform_int_distribution<> random_dist(0,63);
const char* util::random_string(int size) {
    assert(size < 129);
    static char buffer[129];
    for(int i=0;i<size;++i) {
        buffer[i] = random_chars[random_dist(random_genr)];
    }
    buffer[size] = '\0';
    return buffer;
}
