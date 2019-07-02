#include "controller.h"
#include "worker.h"
#include "coroutine.h"

#include "queue.h"
#include "mutex.h"
#include "log/log.h"
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

namespace flame {
    worker* worker::ww_;

    void worker::declare(php::extension_entry& ext) {
        ext
        .on_request_shutdown([] (php::extension_entry& ext) -> bool {
            if (gcontroller->status & controller::STATUS_INITIALIZED) {
                if (php::error::exists()) 
                    std::cerr << "[WARNING] process exited prematurely: uncaught exception / error\n";
                if (!(gcontroller->status & controller::STATUS_RUN)) 
                    std::cerr << "[WARNING] process exited prematurely: missing 'flame\\run();'\n";
                
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
    }

    php::value worker::init(php::parameters& params) {
        php::array options = php::array(0);
        if(params.size() > 1 && params[1].type_of(php::TYPE::ARRAY)) options = params[1];
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
        
        worker::ww_ = new worker();
        gcontroller->init(options);        
        return nullptr;
    }

    php::value worker::go(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        coroutine::start(php::callable([fn = php::callable(params[0])] (php::parameters& params) -> php::value {
            try { // 函数调用产生了堆栈过程，可以捕获异常
                fn.call(); 
            } catch (const php::exception &ex) {
                gcontroller->call_user_cb("exception", {ex}); // 调用用户异常回调
                php::object obj = ex; // 记录错误信息
                gcontroller->output(0) << "[" << time::iso() << "] (FATAL) " << obj.call("__toString") << "\n";

                boost::asio::post(gcontroller->context_x, [] () {
                    gcontroller->status |= controller::STATUS_EXCEPTION;
                    gcontroller->context_x.stop();
                    gcontroller->context_y.stop();
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
        gcontroller->on_user(event, params[1]);
        return nullptr;
    }

    php::value worker::run(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_INITIALIZED) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);

        gcontroller->status |= controller::STATUS_RUN;

        auto tswork = boost::asio::make_work_guard(gcontroller->context_y);
        std::thread ts[4];
        for (int i=0; i<4; ++i) {
            ts[i] = std::thread([] {
                gcontroller->context_y.run();
            });
        }
        gcontroller->context_x.run();
        tswork.reset();
        worker::get()->sw_.close();
        for (int i=0; i<4; ++i) ts[i].join();
        gcontroller->stop();
        return nullptr;
    }

    php::value worker::quit(php::parameters& params) {
        if ((gcontroller->status & controller::STATUS_RUN) == 0)
            throw php::exception(zend_ce_parse_error, "Failed to run flame: exception or missing 'flame\\init()' ?", -1);
        
        gcontroller->call_user_cb("quit");

        gcontroller->context_x.stop();
        gcontroller->context_y.stop();
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

    worker::worker() 
    : lm_(new logger_manager_worker)
    , sw_(gcontroller->context_y) {

        sw_.start(std::bind(&worker::on_signal, this, std::placeholders::_1, std::placeholders::_2));
    }

    void worker::on_signal(const boost::system::error_code error, int sig) {
        if(error) return;
        switch(sig) {
        case SIGINT:
        case SIGTERM:
            if(++close_ > 1) {
                gcontroller->context_x.stop();
                gcontroller->context_y.stop();
                return;
            }
            else coroutine::start(php::callable([] (php::parameters& params) -> php::value { // 通知用户退出(超时后主进程将杀死子进程)
                gcontroller->call_user_cb("quit");
                return nullptr;
            }));
            break;
        case SIGUSR1:
            gcontroller->status ^= controller::STATUS_CLOSECONN;
            break;
        case SIGUSR2: // 日志重载, 与子进程无关
            break;
        }
        // 强制结束时不再重复监听
        sw_.start(std::bind(&worker::on_signal, this, std::placeholders::_1, std::placeholders::_2));
    }
}