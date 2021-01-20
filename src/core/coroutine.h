/********************************************************************
 * php-flame - PHP full stack development framework
 * copyright (c) 2020 terrywh
 ********************************************************************/

#ifndef CPP_CORE_COROUTINE_H
#define CPP_CORE_COROUTINE_H

#include <memory>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/context/fiber.hpp>
#include <thread>

namespace core {
    template <class C>
    class basic_coroutine_runner {
    public:
        using coroutine_type = typename std::decay<C>::type;
        basic_coroutine_runner(coroutine_type* co)
        : co_(co) {
            co_->start();
        }
        ~basic_coroutine_runner() {
            co_->end();
        }
    private:
        std::shared_ptr<coroutine_type> co_;
    };
    // 协程处理器
    template <class C>
    class basic_coroutine_handler {
    public:
        using coroutine_type = typename std::decay<C>::type;
        // 创建空的处理器
        basic_coroutine_handler()
        : count_(nullptr)
        , error_(nullptr) {}
        // 创建指定协程的处理器
        basic_coroutine_handler(std::shared_ptr<coroutine_type> co)
        : count_(nullptr)
        , error_(nullptr)
        , co_(co) {}
        // 复制
        basic_coroutine_handler(const basic_coroutine_handler& ch) = default;
        //
        // 执行器
        boost::asio::executor& executor() {
            return co_->executor();
        }
        // 指定错误返回
        basic_coroutine_handler& operator[](boost::system::error_code& error) {
            error_ = &error;
            return *this;
        }
        // 模拟的回调
        void operator()(const boost::system::error_code& error, std::size_t count = 0) {
            if(error_) *error_ = error;
            if(count_) *count_ = count;
            resume();
        }
        // 重置处理器
        void reset() {
            count_ = nullptr;
            error_ = nullptr;
            co_.reset();
        }
        // 重置处理器用于控制指定的协程
        void reset(std::shared_ptr<coroutine_type> co) {
            count_ = nullptr;
            error_ = nullptr;
            co_ = co;
        }
        // 协程暂停
        inline void yield() {
            co_->yield();
        }
        // 协程恢复
        inline void resume() {
            co_->resume();
        }
    protected:
        std::size_t*                 count_; // 用于捕获数据量
        boost::system::error_code*   error_; // 用于捕获错误值
        // 被管控的协程
        std::shared_ptr<coroutine_type> co_;

        friend coroutine_type;
        friend class boost::asio::async_result<
            basic_coroutine_handler<C>, void (boost::system::error_code error, std::size_t size)>;
    };
    // 协程
    template <class T>
    class basic_coroutine: public std::enable_shared_from_this<basic_coroutine<T>> {
    protected:
        static std::shared_ptr<basic_coroutine<T>> current_;
        using trait_type = typename std::decay<T>::type;
        trait_type             co_;
        boost::context::fiber  c1_;
        boost::context::fiber  c2_;
        boost::asio::executor  ex_;
        boost::asio::executor_work_guard<boost::asio::executor> gd_; // 协程还未运行完毕时，阻止退出
#ifndef NDEBUG // 仅在调试模式计算线程标识
        std::thread::id        id_;
#endif
    public:
        static std::shared_ptr<basic_coroutine<T>> current() {
            return current_;
        }
        // 注意：请使用 go 创建并启动协程
        template <class Executor>
        basic_coroutine(const Executor& ex)
        : ex_(ex)
        , gd_(ex) {
            
        }
        //
        boost::asio::executor& executor() {
            return ex_;
        }
        //
        void start() {
            current_ = this->shared_from_this();
            co_.start();
        }
        // 协程暂停
        void yield() {
#ifndef NDEBUG // 仅在调试模式计算线程标识
            assert(id_ == std::this_thread::get_id());
#endif
            current_ = nullptr;
            co_.yield();
            // 一般当前上下文位于当前执行器（线程）
            c2_ = std::move(c2_).resume();
        }
        // 协程恢复
        void resume() {
            // 恢复实际执行的上下文，可能需要对应执行器（线程）
            boost::asio::post(ex_, [this, self = this->shared_from_this()] () {
                co_.resume();
                current_ = self;
                c1_ = std::move(c1_).resume();
            });
        }
        // 
        void end() {
            current_.reset();
            co_.end();
        }
        // 启动协程
        template <class Executor, class Handler>
        static void start(const Executor& executor, Handler&& fn) {
            auto co = std::make_shared<basic_coroutine<T>>(executor); 
            // 在执行器上运行协程
            boost::asio::post(executor, [co, fn = std::move(fn)] () mutable {
#ifndef NDEBUG // 仅在调试模式计算线程标识
                co->id_ = std::this_thread::get_id();
#endif
                co->c1_ = boost::context::fiber([co, fn = std::move(fn)] (boost::context::fiber&& c2) mutable {
                    co->c2_ = std::move(c2);
                    co->start();
                    fn(); // 注意：协程函数若出现异常，应用程序会被立即结束
                    co->end();
                    return std::move(co->c2_);
                });
                co->c1_ = std::move(co->c1_).resume();
            });
        }
        // template <class T>
        // friend class basic_coroutine_handler;
    };

    class basic_coroutine_traits {
    public:
        basic_coroutine_traits() = default;
        void  start() {}
        void  yield() {}
        void resume() {}
        void    end() {}
    };

    using coroutine = basic_coroutine<basic_coroutine_traits>;
    using coroutine_handler = basic_coroutine_handler<coroutine>;
}

namespace boost::asio {
    template <>
    class async_result<::core::coroutine_handler, void (boost::system::error_code error, std::size_t size)> {
    public:
        explicit async_result(::core::coroutine_handler& ch) : ch_(ch), size_(0) {
            ch_.count_ = &size_;
        }
        using completion_handler_type = ::core::coroutine_handler;
        using return_type = std::size_t;
        return_type get() {
            ch_.yield();
            return size_;
        }
    private:
        ::core::coroutine_handler &ch_;
        std::size_t size_;
    };

    template <>
    class async_result<::core::coroutine_handler, void (boost::system::error_code error)> {
    public:
        explicit async_result(::core::coroutine_handler& ch) : ch_(ch) {
        }
        using completion_handler_type = ::core::coroutine_handler;
        using return_type = void;
        void get() {
            ch_.yield();
        }
    private:
        ::core::coroutine_handler &ch_;
    };
} // namespace boost::asio

#endif // CPP_CORE_COROUTINE_H
