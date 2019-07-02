#include "../controller.h"
#include "log.h"
#include "logger.h"
#include "../coroutine.h"

namespace flame::log {
    
    logger* logger_ = nullptr;

    static php::value trace(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_TRACE, params);
        return nullptr;
    }

    static php::value debug(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_DEBUG, params);
        return nullptr;
    }

    static php::value info(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_INFO, params);
        return nullptr;
    }

    static php::value warning(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_WARNING, params);
        return nullptr;
    }

    static php::value error(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_ERROR, params);
        return nullptr;
    }

    static php::value fatal(php::parameters &params) {
        logger_->write_ex(logger::LEVEL_FATAL, params);
        return nullptr;
    }

    php::value connect(php::parameters& params) {
        php::object obj {php::class_entry<logger>::entry()};
        logger* ptr = static_cast<logger*>(php::native(obj));
        coroutine_handler ch { coroutine::current };
        ptr->lg_ = gcontroller->logger_manager()->connect(params[0], ch);
        return obj;
    }

    void declare(php::extension_entry &ext) {
        gcontroller->on_init([] (const php::array& options) {
            if (options.exists("level")) logger::LEVEL_OPT = options.get("level").to_integer();
            else logger::LEVEL_OPT = logger::LEVEL_TRACE;
            std::string output = "stdout";
            if (options.exists("logger")) output = options.get("logger").to_string();
            logger_ = new logger();
            // 启动的一个更轻量的 C++ 内部协程
            ::coroutine::start(gcontroller->context_x.get_executor(), [output] (::coroutine_handler ch) {
                logger_->set_logger( gcontroller->logger_manager()->connect(output, ch) );
            });
        });
        for(int i=0;i<logger::LEVEL_STR.size();++i) {
            ext.constant({"flame\\log\\" + logger::LEVEL_STR[i], i}); // 日志等级常量
        }
        ext
            .function<trace>("flame\\log\\trace")
            .function<debug>("flame\\log\\debug")
            .function<info>("flame\\log\\info")
            .function<warning>("flame\\log\\warn")
            .function<warning>("flame\\log\\warning")
            .function<error>("flame\\log\\error")
            .function<fatal>("flame\\log\\fail")
            .function<fatal>("flame\\log\\fatal");
    }
}
