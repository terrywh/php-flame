
#include "core.h"
#include "coroutine.h"
#include "../context.h"
#include "../cluster.h"

namespace core { namespace extension {
    
    void core::declare(php::module_entry& entry) {
        entry
            - php::function<core::run>("flame\\run", {
                { php::FAKE_VOID },
                { php::TYPE_ARRAY,    "options"},
                { php::FAKE_CALLABLE, "main"}, // 主协程
            });
        if($context->is_main()) return;
        // 不在主进程注册功能函数
        entry
            - php::function<core::go>("flame\\go", {
                { php::FAKE_VOID },
                { php::FAKE_CALLABLE, "co"}, // 协程函数
            })
            - php::function<core::on>("flame\\on", {
                { php::FAKE_VOID },
                { php::TYPE_STRING,   "event"   },   // event
                { php::FAKE_CALLABLE, "handler" }, // handler
            });
    }
    // 选项
    static void do_option(php::value& options) {
        { // service
            php::value service_name = php::array::get(options, "service.name");
            if(service_name.is(php::TYPE_STRING))
                $context->option.service.name = service_name;

            php::process_title( $context->is_main()
                ? $context->option.service.name + " (php-flame/m)"
                : $context->option.service.name + " (php-flame/w)");
        }
        { // logger
            $context->logger.reset(new ::core::logger());

            php::value logger_target = php::array::get(options, "logger.target");
            if(logger_target.is(php::TYPE_ARRAY)) for(auto& entry: php::cast<php::array>(logger_target)) {
                $context->logger->add_sink( static_cast<std::string_view>(entry.second) );
            }
            else
                $context->logger->add_sink( "console" );
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
        if(!params[0].is(php::TYPE_ARRAY))
            throw php::type_error("A array must be provided as initializing options to the framework.");
        if(!params[1].is(php::FAKE_CALLABLE))
            throw php::type_error("A callable object must be provided as the main coroutine.");
        
        $context->status.ppid = ::getppid();
        $context->status.pid  = ::getpid();
        do_option(params[0]);

        cluster c;
        unsigned int child_process_count = c.evaluate_child_process_count();
        // 主进程
        if($context->is_main()) {
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
            if(php::has_error()) $context->status.state |= context::STATE_ERROR;
        }
        else {
            // 子进程工作线程
            unsigned int worker_thread_count = std::max( // 参照进程数量，计算线程数
                std::thread::hardware_concurrency()/child_process_count, 2u);
            cluster::worker_thread wt {worker_thread_count};
            // handle_signal(nullptr, &wt);
            zend_signal(SIGINT, [] (int sig) {
                std::cout << "main SIGINT\n";
            });
            coroutine_traits::save_context(coroutine_traits::gtx_);
            // 主协程启动
            coroutine::start($context->io_m.get_executor(), params[1]);
            // 注意：上述创建的对象，声明周期在此 if block 范围内；其对应析构会在 run() 后等待、回收相关资源
            $context->io_m.run();
            // 标记状态（尽量在 CPP 侧分离与 PHP 调用关系）
            if(php::has_error()) $context->status.state |= context::STATE_ERROR;
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
