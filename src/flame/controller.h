#pragma once
#include "../vendor.h"

namespace flame {

    class controller {
    public:
        boost::asio::io_context context_x;
        boost::asio::io_context context_y;
        boost::asio::io_context context_z;
        enum class process_type {
            UNKNOWN = 0,
            MASTER = 1,
            WORKER = 2,
        } type;
        boost::process::environment env;
        enum status_t {
            STATUS_UNKNOWN     = 0x00,
            STATUS_INITIALIZED = 0x01,
            // 通知进程退出
            STATUS_CLOSING     = 0x02,
            // 进程正在退出（强制）
            STATUS_QUITING     = 0x04,
            STATUS_RSETING     = 0x08,
            STATUS_EXCEPTION   = 0x10,
            STATUS_RUN         = 0x20,
            STATUS_CLOSECONN   = 0x40,
        };
        int status;
        std::uint8_t   worker_idx;
        std::size_t    worker_size;
        std::size_t    worker_quit; // 多进程退出超时时间
        std::thread::id mthread_id;
    private:
        std::list<std::function<void (const php::array& options)>>  init_cb;
        std::list<std::function<void ()>>                           stop_cb;
        std::unique_ptr<std::multimap<std::string, php::callable>>  evnt_cb; // 防止 PHP 提前回收, 使用堆容器
    public:
        controller();
        controller(const controller& c) = delete;
        controller *on_init(std::function<void(const php::array &options)> fn);
        void init(php::array options);
        controller* on_stop(std::function<void ()> fn);
        void stop();
        controller* add_event(const std::string& event, php::callable cb);
        controller* del_event(const std::string& event);
        std::size_t cnt_event(const std::string& event);
        void event(const std::string& event, std::vector<php::value> params);
        void event(const std::string& event);
    };

    extern std::unique_ptr<controller> gcontroller;

}