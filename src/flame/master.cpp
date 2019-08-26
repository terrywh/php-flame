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

    static std::chrono::steady_clock::time_point tm_init;

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
        if (options.exists("logger")) {
            php::string logger = options.get("logger");
            master::mm_->lg_ = master::mm_->lm_connect({logger.data(), logger.size()});
        }
        else
            master::mm_->lg_ = master::mm_->lm_connect("<clog>");
        // 初始化启动
        gcontroller->init(options);
        master::mm_->ipc_start(); // IPC 启动
        master::mm_->sw_watch(); // 信号监听启动
        tm_init = std::chrono::steady_clock::now();
        return nullptr;
    }

    php::value master::run(php::parameters& params) {
         if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);
            
        gcontroller->status |= controller::STATUS_RUN;
        auto tm_run = std::chrono::steady_clock::now();
        if(tm_run - tm_init < std::chrono::milliseconds(100)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100) - (tm_run - tm_init));
        }
        master::mm_->pm_start(gcontroller->context_x); // 启动工作进程
        
        std::thread ts[2];
        ts[0] = std::thread([] () {
            gcontroller->context_y.run();
        });
        ts[1] = std::thread([] () {
            gcontroller->context_z.run();
        });
        gcontroller->context_x.run();
        master::mm_->sw_close();
        master::mm_->ipc_close();
        master::mm_->lm_close();

        // if (gcontroller->status & controller::STATUS_EXCEPTION) _exit(-1);
        gcontroller->stop();
        ts[0].join();
        ts[1].join();

        master::mm_.reset();
        return nullptr;
    }

    php::value master::dummy(php::parameters& params) {
        return nullptr;
    }

    master::master()
    : master_process_manager(gcontroller->context_y, gcontroller->worker_size, gcontroller->worker_quit)
    , signal_watcher(gcontroller->context_y)
    , master_logger_manager(gcontroller->context_y)
    , master_ipc(gcontroller->context_y) {
        
    }

    std::ostream& master::output() {
        return lg_->stream();
    }
    // !!! 此函数在工作线程中工作
    bool master::on_signal(int sig) {
        switch(sig) {
        break;
        case SIGUSR2: 
            lm_reload(); // 日志重载
        break;
        case SIGUSR1:
            // 由于不再协程上下文暂停，需要共享延长生命周期
            boost::asio::post(gcontroller->context_y.get_executor(), [this, self = shared_from_this()] () {
                gcontroller->status ^= controller::STATUS_CLOSECONN;
                gcontroller->status & controller::STATUS_CLOSECONN ? 
                    (output() << "[" << util::system_time() << "] [INFO] append 'Connection: close' header." << std::endl) :
                    (output() << "[" << util::system_time() << "] [INFO] remove 'Connection: close' header." << std::endl) ;

                pm_kills(SIGUSR1); // 长短连切换
            });
        break;
        case SIGINT:
        case SIGQUIT:
            coroutine::start(gcontroller->context_y.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                // if(gcontroller->status & (controller::STATUS_CLOSING | controller::STATUS_QUITING | controller::STATUS_RSETING)) return; // 关闭进行中
                gcontroller->status |= controller::STATUS_QUITING | controller::STATUS_CLOSECONN;
                pm_close(ch, true); // 立即关闭
            });
            return false; // 除强制停止信号外，需要持续监听信号  
        break;
        case SIGTERM:
            coroutine::start(gcontroller->context_y.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                if(gcontroller->status & (controller::STATUS_CLOSING | controller::STATUS_QUITING | controller::STATUS_RSETING)) return; // 关闭进行中
                gcontroller->status |= controller::STATUS_CLOSING | controller::STATUS_CLOSECONN;
                pm_close(ch); // 超时关闭
            });
        break;
        default:
            if(sig == SIGRTMIN + 1)
                coroutine::start(gcontroller->context_y.get_executor(), [this, self = shared_from_this()] (coroutine_handler ch) { // 使用轻量级的 C++ 内部协程
                    if(gcontroller->status & (controller::STATUS_CLOSING | controller::STATUS_QUITING | controller::STATUS_RSETING)) return; // 关闭进行中
                    gcontroller->status |=   controller::STATUS_RSETING | controller::STATUS_CLOSECONN;
                    pm_reset(ch);
                    gcontroller->status &= ~(controller::STATUS_RSETING | controller::STATUS_CLOSECONN);
                });
        }
        return true;
    }
    // !!! 此函数在工作线程中工作
    bool master::on_message(std::shared_ptr<ipc::message_t> msg, socket_ptr sock) {
        switch(msg->command) {
        // 工作进程创建了新的日志对象，连接到指定的文件输出
        case ipc::COMMAND_LOGGER_CONNECT:
            msg->xdata[0]  = lm_connect({&msg->payload[0], msg->length})->index();
            msg->target = msg->source; // 响应给来源的工作进程
            msg->length = 0;
            ipc_request(msg);
            return true;
        // 工作进程写入日志数据
        case ipc::COMMAND_LOGGER_DATA:
            // 使用 xdata[0] 标识实际写入的目标日志
            lm_get(msg->xdata[0])->stream() << /*"~~~~~" << */std::string_view {&msg->payload[0], msg->length};
            return true;
        case ipc::COMMAND_LOGGER_DESTROY:
            lm_destroy(msg->target);
            return true;
        }
        return master_ipc::on_message(msg, sock);
    }
}
