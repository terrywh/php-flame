#include "master.h"
#include "controller.h"
#include "../util.h"
#include "../master_logger.h"
#include "../master_process.h"

namespace flame {

    std::shared_ptr<master> master::mm_;

    void master::declare(php::extension_entry& ext) {
        ext
        .function<master::init>("flame\\init")
        .function<master::dummy>("flame\\go")
        .function<master::dummy>("flame\\on")
        .function<master::run>("flame\\run");
    }

    php::value master::init(php::parameters& params) {
        php::array options = php::array(0);
        if (params.size() > 1 && params[1].type_of(php::TYPE::ARRAY)) options = params[1];
        if (options.exists("timeout"))
            gcontroller->worker_quit = std::min(std::max(static_cast<int>(options.get("timeout")), 200), 100000);
        else
            gcontroller->worker_quit = 3000;
        
        gcontroller->status |= controller::STATUS_INITIALIZED;
        // 设置进程标题
        std::string title = params[0];
        php::callable("cli_set_process_title").call({title + " (php-flame/m)"});

        // 主进程控制对象
        master::mm_.reset(new master());
        if (options.exists("logger")) 
            master::mm_->lg_ = master::mm_->lm_connect(options.get("logger"));
        else
            master::mm_->lg_ = master::mm_->lm_connect("stdout");
        // 初始化启动
        gcontroller->init(options);
        // 信号监听启动
        master::mm_->sw_watch();
        return nullptr;
    }

    php::value master::run(php::parameters& params) {
         if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);
            
        gcontroller->status |= controller::STATUS_RUN;

        master::get()->pm_start(); // 启动工作进程
        std::thread ts {[] () {
            gcontroller->context_y.run();
        }};
        gcontroller->context_x.run();
        
        master::mm_->sw_close();
        master::mm_->ipc_close();
        master::mm_->lm_close();

        if (gcontroller->status & controller::STATUS_EXCEPTION) _exit(-1);
        else gcontroller->stop();

        ts.join();
        return nullptr;
    }

    php::value master::dummy(php::parameters& params) {
        return nullptr;
    }

    master::master()
    : master_process_manager(gcontroller->context_x, gcontroller->worker_size)
    , signal_watcher(gcontroller->context_y)
    , master_logger_manager()
    , master_ipc(gcontroller->context_y) {
        
    }

    std::ostream& master::output() {
        return lg_->stream();
    }
    // !!! 此函数在工作线程中工作
    bool master::on_signal(int sig) {
        switch(sig) {
        case SIGINT: 
            coroutine::start(gcontroller->context_x.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                pm_reset(ch);
            });
        break;
        case SIGUSR2: 
            lm_reload(); // 日志重载
        break;
        case SIGUSR1:
            if(++stats_ % 2 == 1) 
                output() << "[" << util::system_time() << "] [INFO] append 'Connection: close' header." << std::endl;
            else
                output() << "[" << util::system_time() << "] [INFO] remove 'Connection: close' header." << std::endl;
            pm_kills(SIGUSR1); // 长短连切换
        break;
        case SIGTERM:
            if(++close_ > 1) {
                sw_close();
                coroutine::start(gcontroller->context_x.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                    pm_close(0, ch); // 立即关闭
                });
                return false; // 除强制停止信号外，需要持续监听信号
            }else{
                coroutine::start(gcontroller->context_x.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                    pm_close(gcontroller->worker_quit, ch); // 超时关闭
                });
            }
            break;
        }
        return true;
    }
    // !!! 此函数在工作线程中工作
    bool master::on_message(std::shared_ptr<ipc::message_t> msg, socket_ptr sock) {
        return master_ipc::on_message(msg, sock);
    }
}
