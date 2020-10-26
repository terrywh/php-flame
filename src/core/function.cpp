#include "function.h"

#include "context.hpp"
#include "function_init_options.h"
#include "function_init_cluster.h"
#include "function_run_worker.h"

#include <boost/process.hpp>

namespace flame { namespace core {

    using init_options = function_init_options;
    using init_cluster = function_init_cluster;
    using run_worker = function_run_worker;
    // 框架初始化
    static php::value init(php::parameters& params) {
        std::string name = params[0];
        
        if($->env.count("FLAME_CUR_WORKER") > 0) {
            php::process_title( name + " (php-flame/w)");
            // 工作进程，初始化配置
            init_options io {params};
            // 触发事件
            $->on_flame_init();
        }
        else {
            php::process_title( name + " (php-flame/m)");
            // 在主进程启动工作进程并等待其结束
            init_cluster ic;
            // TODO 监听信号
            $->io_m.run();
            // 不在主进程执行任何逻辑功能，直接结束进程
            exit(0);
        }
        return nullptr;
    }
    // 启动
    static php::value run(php::parameters& params) {
        run_worker w;
        // worker_.start();
        $->io_m.run();
        return nullptr;
    }
    // 
    static php::value test(php::parameters& params) {
        // url u("protocol://name@pass@host:1234/path?a=b");
        // std::cout << u.scheme() << " / " << u << std::endl;
        // u.scheme("http").query().set("x", "y");
    
        // std::cout << u.str(false) << " / " << u.str(true) << std::endl;

        // std::cout << static_cast<std::string>(random_string(16)) << std::endl;
        // std::cout << static_cast<std::string>($clock) << ": " << static_cast<int>($clock) << std::endl;
        std::cout << "dddddddddd" << std::endl;
        int x = 100;
        std::cout << x << std::endl;
        return nullptr;
    }
    // 强制停止
    static void stop() {
        $->io_w.stop();
        $->io_m.stop();
    }
    // 
    void function::declare(php::module_entry& module) {
        module
            - php::module_shutdown([] (php::module_entry& module) {
                // stop();
            })
            - php::function<flame::core::init>("flame\\init", {
                {"name", php::TYPE_STRING, /* byref */false, /* null */false},
                {"opts", php::TYPE_ARRAY, /* byref */false, /* null */true},
            })
            - php::function<flame::core::run>("flame\\run");
    }
}}