
#include "core.h"
#include "../context.h"
#include "../cluster.h"
#include <sstream>

namespace core { namespace extension {
    
    void core::declare(php::module_entry& entry) {
        entry
            - php::function<core::run>("flame\\run", {
                {"opts", php::TYPE_ARRAY},
                {"main", php::FAKE_CALLABLE},
            })
            - php::function<core::go>("flame\\go", {
                {"routine", php::FAKE_CALLABLE},
            })
            - php::function<core::on>("flame\\on", {
                {"event", php::TYPE_STRING},
                {"handler", php::FAKE_CALLABLE},
            });
    }
    // 填充选项
    static void write_ctx_opt(php::array opt) {
        php::value& service = opt["service"];
        if(service.is(php::TYPE_ARRAY)) {
            $context->opt.service.name = service.as<php::array>().get("name");
        }
    }
    // 命令行
    static std::string cmd() {
        std::stringstream ss;
        for(const char* arg : php::runtime::argv()) ss << arg << " ";
        return ss.str();
    }
    // 框架启动，可选的设置部分选项
    php::value core::run(php::parameters& params) {
        if(params[0].is(php::TYPE_ARRAY)) write_ctx_opt(params[0]);

        $context->env.ppid = ::getppid();
        $context->env.pid = ::getpid();

        cluster c { cmd() };

        if(c.is_main()) php::process_title($context->opt.service.name + " (php-flame/w)");
        else php::process_title($context->opt.service.name + " (php-flame/w)");
        $context->io_m.run();

        // 标记状态（尽量在 CPP 侧分离与 PHP 调用关系）
        if(php::has_error()) $context->env.status |= context::STATUS_ERROR;
        return nullptr;
    }

    php::value core::go(php::parameters& params) {
        // TODO
        return nullptr;
    }

    php::value core::on(php::parameters& params) {
        // TODO
        return nullptr;
    }


    
}}
