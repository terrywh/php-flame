#include "master.h"
#include "controller.h"
#include "../util.h"

namespace flame {

    master* master::mm_;

    void master::declare(php::extension_entry& ext) {
        ext
        .function<master::init>("flame\\init")
        .function<master::dummy>("flame\\go")
        .function<master::dummy>("flame\\on")
        .function<master::run>("flame\\run");
    }

    php::value master::init(php::parameters& params) {
        php::array options = php::array(0);
        if(params.size() > 1 && params[1].type_of(php::TYPE::ARRAY)) options = params[1];
        if (options.exists("timeout"))
            gcontroller->worker_quit = std::min(std::max(static_cast<int>(options.get("timeout")), 200), 100000);
        else
            gcontroller->worker_quit = 3000;
        
        gcontroller->status |= controller::STATUS_INITIALIZED;
        // 设置进程标题
        std::string title = params[0];
        php::callable("cli_set_process_title").call({title + " (php-flame/m)"});
        // 主进程控制对象
        master::mm_ = new master();
        // 初始化启动
        gcontroller->init(options);
        return nullptr;
    }

    php::value master::run(php::parameters& params) {
         if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);
            
        gcontroller->status |= controller::STATUS_RUN;
        // 启动工作进程
        master::get()->pm_.start();
        std::thread ts {[] () {
            gcontroller->context_y.run();
        }};
        gcontroller->context_x.run();

        if (gcontroller->status & controller::STATUS_EXCEPTION) _exit(-1);
        else gcontroller->stop();

        ts.join();
        return nullptr;
    }

    php::value master::dummy(php::parameters& params) {
        return nullptr;
    }

    master::master()
    : lm_(new logger_manager_master)
    , pm_(gcontroller->context_x, gcontroller->worker_size)
    , sw_(gcontroller->context_y) {
        
        sw_.start(std::bind(&master::on_signal, this, std::placeholders::_1, std::placeholders::_2));
    }

    void master::on_signal(const boost::system::error_code& error, int sig) {
        if (error) return;
        switch(sig) {
        case SIGINT: 
            coroutine::start(gcontroller->context_x.get_executor(), [this] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                pm_.restart(ch);
            });
        break;
        case SIGUSR2: 
            lm_->reload(); // 日志重载
        break;
        case SIGUSR1:
            if(++stats_ % 2 == 1) 
                lm_->index(0)->stream() << "[" << util::system_time() << "] [INFO] append 'Connection: close' header." << std::endl;
            else
                lm_->index(0)->stream() << "[" << util::system_time() << "] [INFO] remove 'Connection: close' header." << std::endl;
            pm_.signal(SIGUSR1); // 长短连切换
        break;
        case SIGTERM:
            if(++close_ > 1) {
                sw_.close();
                coroutine::start(gcontroller->context_x.get_executor(), [this] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                    pm_.close(true, ch);
                });
                return;
            }else{
                coroutine::start(gcontroller->context_x.get_executor(), [this] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                    pm_.close(false, ch);
                });
            }
            break;
        }
        // 除强制停止信号外，需要持续监听信号
        sw_.start(std::bind(&master::on_signal, this, std::placeholders::_1, std::placeholders::_2));
    }
}
