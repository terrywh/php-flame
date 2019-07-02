#pragma once
#include "../vendor.h"
#include "../logger.h"

class controller {
public:
    boost::asio::io_context context_x;
    boost::asio::io_context context_y;
    enum class process_type {
        UNKNOWN = 0,
        MASTER = 1,
        WORKER = 2,
    } type;
    boost::process::environment env;
    enum status_t {
        STATUS_UNKNOWN     = 0x00,
        STATUS_INITIALIZED = 0x01,
        STATUS_SHUTDOWN    = 0x02,
        STATUS_EXCEPTION   = 0x04,
        STATUS_RUN         = 0x08,
        STATUS_CLOSECONN   = 0x10,
    };
    int status;
    std::size_t    worker_size;
    std::size_t    worker_quit; // 多进程退出超时时间
    std::thread::id mthread_id;
    // 日志管理器
    logger_manager*         lm;
private:
    std::list<std::function<void (const php::array& options)>>  init_cb;
    std::list<std::function<void ()>>                           stop_cb;
    std::multimap<std::string, php::callable>*                  user_cb; // 防止 PHP 提前回收, 使用堆容器
public:
    controller();
    controller(const controller& c) = delete;
    void init(php::array options);
    void stop();
    controller *on_init(std::function<void(const php::array &options)> fn);
    controller* on_stop(std::function<void ()> fn);
    controller* on_user(const std::string& event, php::callable cb);
    void call_user_cb(const std::string& event, std::vector<php::value> params);
    void call_user_cb(const std::string& event);
    // TODO write_to_master | write_worker
};

extern std::unique_ptr<controller> gcontroller;
