#include "log.h"
#include "logger.h"
#include "../time/time.h"
#include "../../logger.h"
#include "../controller.h"

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

    logger::~logger() {
        gcontroller->logger_manager()->destroy(lg_);
    }

    void logger::write_ex(int lv, php::parameters& params) {
        if (lv < LEVEL_OPT) return;
        std::ostream& os = lg_ ? lg_->stream() : std::cout;
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
