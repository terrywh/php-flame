#include "../util.h"
#include "../worker_logger.h"
#include "controller.h"
#include "worker.h"
#include "coroutine.h"

#include "queue.h"
#include "mutex.h"
#include "log/log.h"
#include "log/logger.h"
#include "os/os.h"
#include "time/time.h"
#include "mysql/mysql.h"
#include "redis/redis.h"
#include "mongodb/mongodb.h"
#include "kafka/kafka.h"
#include "rabbitmq/rabbitmq.h"
#include "tcp/tcp.h"
#include "udp/udp.h"
#include "http/http.h"
#include "hash/hash.h"
#include "encoding/encoding.h"
#include "compress/compress.h"
#include "toml/toml.h"
#include "smtp/smtp.h"

namespace flame {
    std::shared_ptr<worker> worker::ww_;

    void worker::declare(php::extension_entry& ext) {
        ext
        .on_request_shutdown([] (php::extension_entry& ext) -> bool {
            if (gcontroller->status & controller::STATUS_INITIALIZED) {
                // if (php::error::exists()) 
                //     log::logger_->stream() << "[" << util::system_time() << "] (WARNING) unexpected process exit: uncaught exception / error\n" << std::endl;
                if (!(gcontroller->status & controller::STATUS_RUN)) 
                    log::logger_->stream() << "[" << util::system_time() << "] (WARNING) unexpected process exit: missing 'flame\\run();'" << std::endl;
                
                gcontroller->status & controller::STATUS_EXCEPTION ? _exit(-1) : _exit(0);
            }
            return true;
        })
        .function<worker::init>("flame\\init", {
            {"process_name", php::TYPE::STRING},
            {"options", php::TYPE::ARRAY, false, true},
        })
        
        .function<worker::go>("flame\\go", {
            {"coroutine", php::TYPE::CALLABLE},
        })
        .function<worker::on>("flame\\on", {
            {"event", php::TYPE::STRING},
            {"callback", php::TYPE::CALLABLE},
        })
        .function<worker::off>("flame\\off", {
            {"event", php::TYPE::STRING},
        })
        .function<worker::run>("flame\\run")
        .function<worker::quit>("flame\\quit")
        .function<worker::co_id>("flame\\co_id")
        .function<worker::co_ct>("flame\\co_ct")
        .function<worker::co_ct>("flame\\co_count")
        .function<worker::set>("flame\\set", {
            {"target", php::TYPE::ARRAY, true},
            {"fields", php::TYPE::STRING},
            {"values", php::TYPE::UNDEFINED},
        })
        .function<worker::get>("flame\\get", {
            {"target", php::TYPE::ARRAY},
            {"fields", php::TYPE::STRING},
        })
        .function<worker::send>("flame\\send", {
            {"target", php::TYPE::INTEGER},
            {"data", php::TYPE::UNDEFINED},
        })
        .function<worker::unique_id>("flame\\unique_id", {
            {"node", php::TYPE::INTEGER},
        });
        // 顶级命名空间
        flame::mutex::declare(ext);
        flame::queue::declare(ext);
        // 子命名空间模块注册
        flame::log::declare(ext);
        flame::os::declare(ext);
        flame::time::declare(ext);
        flame::mysql::declare(ext);
        flame::redis::declare(ext);
        flame::mongodb::declare(ext);
        flame::kafka::declare(ext);
        flame::rabbitmq::declare(ext);
        flame::tcp::declare(ext);
        flame::udp::declare(ext);
        flame::http::declare(ext);
        flame::hash::declare(ext);
        flame::encoding::declare(ext);
        flame::compress::declare(ext);
        flame::toml::declare(ext);
        flame::smtp::declare(ext);
    }

    php::value worker::init(php::parameters& params) {
        php::array options = php::array(0);
        if (params.size() > 1 && params[1].type_of(php::TYPE::ARRAY))
            options = params[1];
        if (options.exists("timeout"))
            gcontroller->worker_quit = std::min(std::max(static_cast<int>(options.get("timeout")), 200), 100000);
        else
            gcontroller->worker_quit = 3000;
        
        gcontroller->status |= controller::STATUS_INITIALIZED;
        // 设置进程标题
        std::string title = params[0];
        if (gcontroller->worker_size > 0) {
            std::string index = gcontroller->env["FLAME_CUR_WORKER"].to_string();
            php::callable("cli_set_process_title").call({title + " (php-flame/" + index + ")"});
        }
        else
            php::callable("cli_set_process_title").call({title + " (php-flame/w)"});

        worker::ww_.reset(new worker(gcontroller->worker_idx));

        // 信号监听启动
        worker::ww_->sw_watch();
        if(gcontroller->worker_size > 0) worker::ww_->ipc_start();

        gcontroller->init(options); // 首个 logger 的初始化过程在 logger 注册 on_init 回调中进行（异步的）
        return nullptr;
    }

    php::value worker::go(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        coroutine::start(php::callable([fn = php::callable(params[0])] (php::parameters& params) -> php::value {
            try { // 函数调用产生了堆栈过程，可以捕获异常
                fn.call(); 
            } catch (const php::exception &ex) {
                gcontroller->event("exception", {ex}); // 调用用户异常回调
                php::object obj = ex; // 记录错误信息
                log::logger_->stream() << "[" << time::iso() << "] (FATAL) " << obj.call("__toString") << std::endl;

                boost::asio::post(gcontroller->context_x, [] () {
                    gcontroller->status |= controller::STATUS_EXCEPTION;
                    gcontroller->context_x.stop();
                });
            }
            return nullptr;
        }));
        // 如下形式无法在内部无法捕获到 PHP 错误（顶层栈）
        // flame::coroutine::start(params[0]);
        return nullptr;
    }

    php::value worker::on(php::parameters &params) {

        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        std::string event = params[0].to_string();
        if (!params[1].type_of(php::TYPE::CALLABLE)) throw php::exception(zend_ce_type_error, "Failed to set callback: callable required", -1);
        // message 事件需要启动协程辅助，故需要对应的停止机制
        if(event == "message" && gcontroller->cnt_event(event) == 0) {
            worker::ww_->msg_start();
        }
        gcontroller->add_event(event, params[1]);
        return nullptr;
    }

    php::value worker::off(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        std::string event = params[0].to_string();
        gcontroller->del_event(event);
        // message 事件启动了额外的协程，需要停止
        if(event == "message"/*&& gcontroller->cnt_event(event) == 0 */) {
            worker::ww_->msg_close();
        }
        return nullptr;
    }

    php::value worker::run(php::parameters& params) {
        if(params.size() > 0) {
            worker::go(params);
        }
        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        gcontroller->status |= controller::STATUS_RUN;

        auto ywork = boost::asio::make_work_guard(gcontroller->context_y);
        auto zwork = boost::asio::make_work_guard(gcontroller->context_z);
        std::thread ts[4];
        for (int i=0; i<3; ++i) {
            ts[i] = std::thread([] {
                gcontroller->context_y.run();
            });
        }
        ts[3] = std::thread([] {
            gcontroller->context_z.run();
        });
        gcontroller->context_x.run();
        gcontroller->stop();
        
        ywork.reset();
        zwork.reset();
        worker::ww_->sw_close();
        worker::ww_->ipc_close();
        worker::ww_->lm_close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // 强制退出立即结束工作线程
        if(gcontroller->status & controller::STATUS_QUITING) {
            gcontroller->context_y.stop();
            gcontroller->context_z.stop();
        }
        // 等待工作线程结束
        for (int i=0; i<4; ++i) ts[i].join();
        worker::ww_.reset();
        // gcontroller.reset();
        return nullptr;
    }

    php::value worker::quit(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_RUN) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        gcontroller->context_x.stop();
        return nullptr;
    }

    php::value worker::co_id(php::parameters& params) {
        return reinterpret_cast<uintptr_t>(coroutine::current.get());
    }

    php::value worker::co_ct(php::parameters& params) {
        return coroutine::count;
    }

    php::value worker::get(php::parameters& params) {
        php::array  target = params[0];
        php::string fields = params[1];
        return flame::toml::get(target, { fields.data(), fields.size() }, 0);
    }

    php::value worker::set(php::parameters& params) {
        php::array  target = params.get(0, true);
        php::string fields = params[1];
        php::value  values = params[2];
        flame::toml::set(target, {fields.data(), fields.size()}, 0, values);
        return nullptr;
    }

    php::value worker::send(php::parameters& params) {
        worker::ww_->ipc_notify(static_cast<int>(params[0]), params[1]);
        return nullptr;
    }

    php::value worker::unique_id(php::parameters& params) {
        static std::atomic_int16_t incr = 0;
        int node = static_cast<int>(params[0]) % 1024;
        std::int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(
                time::now().time_since_epoch()).count(),
            epoch = 0;
        if (params.size() > 1) 
            epoch = params[1].to_integer();
        if (epoch <= 1000000000000l || epoch > time)
            epoch = 1288834974657l;

        time -= epoch;
        std::int64_t sfid = 
            ((time & 0x01ffffffffffl) << 22) | // 41 bit
            ((node & 0x03ff) << 12) | // 10bit
            (incr++ & 0x0fff); // 12bit;
        return sfid;
    }

    worker::worker(std::uint8_t idx)
    : signal_watcher(gcontroller->context_z)
    , worker_ipc(gcontroller->context_z, idx)
    , worker_logger_manager(this) {

    }

    std::ostream& worker::output() {
        return log::logger_ ? log::logger_->stream() : std::clog;
    }

    // !!! 此函数在工作线程中运行
    bool worker::on_signal(int sig) {
SIGNAL_AGAIN:
        switch(sig) {
        case SIGINT:
            if(gcontroller->worker_size > 0) break; // 多进程模式忽略
            [[fallthrough]]; // 单进程模式同 SIGQUIT 处理: 强制停止
        case SIGQUIT:
            gcontroller->status |= controller::STATUS_CLOSING | controller::STATUS_QUITING;
            boost::asio::post(gcontroller->context_x, [] () {
                gcontroller->context_x.stop();
            });
            return false; // 强制停止, 仅允许进行一次; 停止信号处理
        case SIGTERM:
            // 单进程模式同强制退出
            if(gcontroller->worker_size == 0) {
                sig = SIGQUIT;
                goto SIGNAL_AGAIN;
            }
            gcontroller->status |= controller::STATUS_CLOSING;
            coroutine::start(php::callable([] (php::parameters& params) -> php::value { // 通知用户退出(超时后主进程将杀死子进程)
                gcontroller->event("quit");
                return nullptr;
            }));
            break;
        case SIGUSR1:
            boost::asio::post(gcontroller->context_x, [] () {
                gcontroller->status ^= controller::STATUS_CLOSECONN;
            });
            break;
        case SIGUSR2: // 日志重载, 与子进程无关
            break;
        }
        return true;
    }
    // !!! 此函数在工作线程中工作
    bool worker::on_message(std::shared_ptr<ipc::message_t> msg) {
        switch(msg->command) {
        case ipc::COMMAND_MESSAGE_STRING:
            boost::asio::post(gcontroller->context_x, [this, self = worker::ww_, msg] () {
                msgq_.push_back(php::string(&msg->payload[0], msg->length));
                if(msgq_.size() == 1 && msgc_) msgc_.resume();
            });
        break;
        case ipc::COMMAND_MESSAGE_JSON: // !!! json_decode('"abc"') === null !!!
            boost::asio::post(gcontroller->context_x, [this, self = worker::ww_, msg] () {
                msgq_.push_back(php::json_decode(&msg->payload[0], msg->length));
                if(msgq_.size() == 1 && msgc_) msgc_.resume();
            });
        break;
        }
        return true;
    }

    void worker::msg_start() {
        coroutine::start(php::callable([this, self = worker::ww_] (php::parameters& params) -> php::value {
            coroutine_handler ch {coroutine::current};
            while(true) {
                while(msgq_.empty()) {
                    msgc_.reset(ch);
                    msgc_.suspend();
                    msgc_.reset();
                }
                php::value v = msgq_.front();
                msgq_.pop_front();

                if(v.type_of(php::TYPE::UNDEFINED)) break;
                gcontroller->event("message", {v});
            }
            return nullptr;
        }));
    }

    void worker::msg_close() {
        msgq_.push_back(php::value());
        if(msgq_.size() == 1 && msgc_) msgc_.resume();
    }
}