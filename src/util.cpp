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
struct random_reader {
    char o1:6;
    char o2:6;
    char o3:6;
    char o4:6;
};
static std::default_random_engine random_genr((unsigned long)(&random_chars));
static std::uniform_int_distribution<std::uint64_t> random_dist;
const char* util::random_string(int size) {
    assert(size < 129);
    assert(sizeof(random_reader) == 3);
    static char target[128], source[96];
    int rand = std::ceil(size * 6 / 8.0);
    
    for(int i=0;i<rand;++i) {
        std::uint64_t* u = (std::uint64_t*)&source[i * 8];
        *u = random_dist(random_genr);
    }
    int j = 0;
    for(int i=0;i<rand*8 && i<96;i+=3) {
        random_reader* rr = (random_reader*)&source[i];
        target[j++] = random_chars[rr->o1 & 0x3f];
        target[j++] = random_chars[rr->o2 & 0x3f];
        target[j++] = random_chars[rr->o3 & 0x3f];
        target[j++] = random_chars[rr->o4 & 0x3f];
    }
    return target;
}