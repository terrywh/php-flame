#include "../../worker_logger.h"
#include "log.h"
#include "logger.h"
#include "../worker.h"
#include "../time/time.h"
#include "../coroutine.h"

namespace flame::log {
    
    std::array<std::string, 6> logger::LEVEL_STR = {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARNING",
        "ERROR",
        "FATAL",
    };
    int logger::LEVEL_OPT = logger::LEVEL_TRACE;

    logger* logger_ = nullptr;

    void logger::declare(php::extension_entry& ext) {
        gcontroller->on_init([] (const php::array& options) {
            if (options.exists("level")) logger::LEVEL_OPT = options.get("level").to_integer();
            else logger::LEVEL_OPT = logger::LEVEL_TRACE;
            std::string output = "stdout";
            if (options.exists("logger")) output = options.get("logger").to_string();
            logger_ = new logger();
            // 启动的一个更轻量的 C++ 内部协程
            ::coroutine::start(gcontroller->context_x.get_executor(), [output] (::coroutine_handler ch) {
                logger_->connect(output, ch);
            });
        });
        for(int i=0;i<logger::LEVEL_STR.size();++i) {
            ext.constant({"flame\\log\\" + logger::LEVEL_STR[i], i}); // 日志等级常量
        }

        php::class_entry<logger> class_logger("flame\\log\\logger");
        class_logger
            .method<&logger::trace>("trace")
            .method<&logger::debug>("debug")
            .method<&logger::info>("info")
            .method<&logger::warning>("warn")
            .method<&logger::warning>("warning")
            .method<&logger::error>("error")
            .method<&logger::fatal>("fail")
            .method<&logger::fatal>("fatal");
        ext.add(std::move(class_logger));
    }
    
    void logger::connect(const std::string& path, ::coroutine_handler& ch) {
        lg_ = worker::get()->lm_connect(path, ch);
    }
    
    std::ostream& logger::stream() {
        return lg_ ? lg_->stream() : std::clog; // 由于 lg_ 的填充是异步的，须防止其还未初始化就被其他模块访问
    }

    void logger::write_ex(int lv, php::parameters& params) {
        if (lv < LEVEL_OPT) return;
        std::ostream& os = logger::stream();
        os << '[' << time::iso() << "] (";
        os << LEVEL_STR[lv];
        os << ")";
        for (int i = 0; i < params.size(); ++i)
            os << ' ' << params[i].ptr();

        os << std::endl;
    }

    php::value logger::trace(php::parameters &params) {
        write_ex(LEVEL_TRACE, params);
        return nullptr;
    }

    php::value logger::debug(php::parameters &params) {
        write_ex(LEVEL_DEBUG, params);
        return nullptr;
    }

    php::value logger::info(php::parameters &params) {
        write_ex(LEVEL_INFO, params);
        return nullptr;
    }

    php::value logger::warning(php::parameters &params) {
        write_ex(LEVEL_WARNING, params);
        return nullptr;
    }

    php::value logger::error(php::parameters &params) {
        write_ex(LEVEL_ERROR, params);
        return nullptr;
    }

    php::value logger::fatal(php::parameters &params) {
        write_ex(LEVEL_FATAL, params);
        return nullptr;
    }
}
