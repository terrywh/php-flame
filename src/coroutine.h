#pragma once
#include "vendor.h"

class coroutine: public std::enable_shared_from_this<coroutine> {
public:
    coroutine(boost::asio::executor ex)
    : ex_(ex) {
        
    }
    virtual ~coroutine() {

    }
    // static std::shared_ptr<coroutine> current;
    template <class Handler>
    static void start(boost::asio::executor ex, Handler&& fn) {
        auto co = std::make_shared<coroutine>(ex);
        
        boost::asio::post(co->ex_, [co, fn] () mutable {
            co->c1_ = boost::context::fiber([co, gd = boost::asio::make_work_guard(co->ex_), fn] (boost::context::fiber&& c2) mutable {
                co->c2_ = std::move(c2);
                fn({co});
                return std::move(co->c2_);
            });
            co->c1_ = std::move(co->c1_).resume();
        });
    }
    virtual void suspend() {
        c2_ = std::move(c2_).resume();
    }
    virtual void resume() {
        boost::asio::post(ex_, [co = shared_from_this()] () {
            co->c1_ = std::move(co->c1_).resume();
        });
    }
protected:
    boost::context::fiber c1_;
    boost::context::fiber c2_;
    boost::asio::executor ex_;

    friend class coroutine_handler;
};

class coroutine_handler {
public:
    coroutine_handler() = default;
    coroutine_handler(std::shared_ptr<coroutine> co)
    : co_(co) { }
    coroutine_handler(const coroutine_handler& ch) = default;
    virtual ~coroutine_handler() {

    }
    coroutine_handler& operator[](boost::system::error_code& error) {
        error_ = &error;
        return *this;
    }
    coroutine_handler& operator[](std::size_t& size) {
        size_ = &size;
        return *this;
    }
    
    inline void operator()(const boost::system::error_code& error, std::size_t size = 0) {
        if(error_) *error_ = error;
        if(size_) *size_ = size;
        co_->resume();
    }

    inline bool operator !() {
        return co_ == nullptr;
    }
    
    inline operator bool() {
        return co_ != nullptr;
    }

    void reset() {
        co_.reset();
        size_ = nullptr;
        error_= nullptr;
    }
    void reset(std::shared_ptr<coroutine> co) {
        co_ = co;
        size_ = nullptr;
        error_= nullptr;
    }
    void reset(coroutine_handler& ch) {
        co_ = ch.co_;
        size_ = nullptr;
        error_= nullptr;
    }

    inline void suspend() {
        co_->suspend();
    }
    inline void resume() {
        co_->resume();
    }

    void error(const boost::system::error_code& error) {
        if(error_) *error_ = error;
    }
    void size(std::size_t sz) {
        if(size_) *size_ = sz;
    }
public:
    std::size_t*                size_ = nullptr;
    boost::system::error_code* error_ = nullptr;
    std::shared_ptr<coroutine>    co_;

private:

};

namespace boost::asio {
    template <>
    class async_result<::coroutine_handler, void (boost::system::error_code error, std::size_t size)> {
    public:
        explicit async_result(::coroutine_handler& ch) : ch_(ch), size_(0) {
            ch_.size_ = &size_;
        }
        using completion_handler_type = ::coroutine_handler;
        using return_type = std::size_t;
        return_type get() {
            ch_.suspend();
            return size_;
        }
    private:
        ::coroutine_handler &ch_;
        std::size_t size_;
    };

    template <>
    class async_result<::coroutine_handler, void (boost::system::error_code error)> {
    public:
        explicit async_result(::coroutine_handler& ch) : ch_(ch) {
        }
        using completion_handler_type = ::coroutine_handler;
        using return_type = void;
        void get() {
            ch_.suspend();
        }
    private:
        ::coroutine_handler &ch_;
    };
} // namespace boost::asio
