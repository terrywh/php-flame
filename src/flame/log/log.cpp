#include "../coroutine.h"
#include "../worker.h"
#include "log.h"
#include "logger.h"

namespace flame::log {
    
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
        coroutine_handler ch {coroutine::current};
        php::object obj {php::class_entry<logger>::entry()};
        logger* ptr = static_cast<logger*>(php::native(obj));
        ptr->connect(params[0], ch);
        return obj;
    }

    void declare(php::extension_entry &ext) {
        ext
            .function<trace>("flame\\log\\trace")
            .function<debug>("flame\\log\\debug")
            .function<info>("flame\\log\\info")
            .function<warning>("flame\\log\\warn")
            .function<warning>("flame\\log\\warning")
            .function<error>("flame\\log\\error")
            .function<fatal>("flame\\log\\fail")
            .function<fatal>("flame\\log\\fatal");
        
        logger::declare(ext);
    }
}
