#include "time.h"
#include "../controller.h"
#include "../coroutine.h"

namespace flame::time
{

    static std::chrono::time_point<std::chrono::system_clock> time_system;
	static std::chrono::time_point<std::chrono::steady_clock> time_steady;

    static php::value sleep(php::parameters& params)
    {
        coroutine_handler ch {coroutine::current};
        boost::asio::steady_timer tm(gcontroller->context_x);
        int ms = static_cast<int>(params[0]);
        if(ms < 0) ms = 1;
        tm.expires_from_now(std::chrono::milliseconds(ms));
        tm.async_wait(ch);
        return nullptr;
    }


    std::chrono::time_point<std::chrono::system_clock> now()
    {
        std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - time_steady);
        return time_system + diff;
    }
    php::string iso(const std::chrono::time_point<std::chrono::system_clock>& now) {
        php::string data(19);
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        struct tm *m = std::localtime(&t);
        sprintf(data.data(), "%04d-%02d-%02d %02d:%02d:%02d",
                1900 + m->tm_year,
                1 + m->tm_mon,
                m->tm_mday,
                m->tm_hour,
                m->tm_min,
                m->tm_sec);
        return std::move(data);
    }
    php::string iso()
    {
        return iso(now());
    }

    static php::value now(php::parameters &params)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now().time_since_epoch()).count();
    }
    static php::value iso(php::parameters &params)
    {
        return iso();
    }

    void declare(php::extension_entry& ext)
    {
        time_steady = std::chrono::steady_clock::now();
		time_system = std::chrono::system_clock::now();

        ext
        .function<sleep>("flame\\time\\sleep",
        {
            {"duration", php::TYPE::INTEGER},
        })
        // 毫秒时间戳
        .function<now>("flame\\time\\now")
        // 标准时间 YYYY-mm-dd HH:ii:ss
        .function<iso>("flame\\time\\iso");

    }

}
