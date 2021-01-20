
#include "core.h"
#include "coroutine.h"
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
        php::value& service = opt.get("service");
        if(service.is(php::TYPE_ARRAY)) {
            $context->opt.service.name = service.as<php::array>().get("name");
        }
    }
    // 命令行
    static std::string command_line() {
        std::stringstream ss;
        for(const char* arg : php::runtime::argv()) ss << arg << " ";
        return ss.str();
    }
    // 框架启动，可选的设置部分选项
    php::value core::run(php::parameters& params) {
        if(params[0].is(php::TYPE_ARRAY)) write_ctx_opt(params[0]);
        if(!params[1].is(php::FAKE_CALLABLE)) 
            throw php::type_error("A callable object must be provided as the main coroutine.");

        $context->env.ppid = ::getppid();
        $context->env.pid = ::getpid();

        cluster c;
        unsigned int child_process_count = c.evaluate_child_process_count();
        // 主进程
        if(c.is_main()) {
            php::process_title($context->opt.service.name + " (php-flame/w)");
            // 启动子进程
            cluster::child_process cp { child_process_count, command_line() };
            cluster::worker_thread wt { 1 };

            // handle_signal(&cp, &wt);
            zend_signal(SIGINT, [] (int sig) {
                std::cout << "worker SIGINT\n";
            });
            // 注意：上述创建的对象，声明周期在此 if block 范围内；其对应析构会在 run() 后等待、回收相关资源
            $context->io_m.run();
            // 标记状态（尽量在 CPP 侧分离与 PHP 调用关系）
            if(php::has_error()) $context->env.status |= context::STATUS_ERROR;
        }
        else {
            php::process_title($context->opt.service.name + " (php-flame/w)");
            // 子进程工作线程
            unsigned int worker_thread_count = std::max( // 参照进程数量，计算线程数
                std::thread::hardware_concurrency()/child_process_count, 2u);
            cluster::worker_thread wt {worker_thread_count};

            // handle_signal(nullptr, &wt);
            zend_signal(SIGINT, [] (int sig) {
                std::cout << "main SIGINT\n";
            });
            // 主协程启动
            coroutine::start($context->io_m.get_executor(), params[1]);
            // 注意：上述创建的对象，声明周期在此 if block 范围内；其对应析构会在 run() 后等待、回收相关资源
            $context->io_m.run();
            // 标记状态（尽量在 CPP 侧分离与 PHP 调用关系）
            if(php::has_error()) $context->env.status |= context::STATUS_ERROR;
        }
        return nullptr;
    }

    php::value core::go(php::parameters& params) {
        coroutine::start($context->io_m.get_executor(), params[0]);
        return nullptr;
    }

    php::value core::on(php::parameters& params) {
        // TODO
        return nullptr;
    }


    
}}
