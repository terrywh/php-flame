#include "../controller.h"
#include "../coroutine.h"
#include "_client.h"
#include "message.h"

namespace flame::rabbitmq
{
    static bool str2bool(const char *str){
        return strncasecmp(str, "1", 1) == 0 || strncasecmp(str, "yes", 3) == 0 ||
            strncasecmp(str, "yes", 3) == 0 || strncasecmp(str, "true", 4);
    }

#define CHECK_AND_SET_FLAG(xflag, yflag)                        \
    for (auto i = u.query.find(#xflag); i != u.query.end();) {  \
        const char *str = i->second.c_str();                    \
        if (str2bool(str)) fl_ |= yflag;                      \
    }

    _client::_client(url u, coroutine_handler& ch)
    : chl_(gcontroller->context_x)
    , con_(&chl_, AMQP::Address(u.str(true, false).c_str(), u.str().size()))
    , chn_(&con_)
    , tm_(gcontroller->context_x)
    , producer_cb_(false) {
        chn_.onReady([this, &ch]() {
            // ch_.onError([] (const char* message) {});
            chn_.onError([this] (const char* message) {
                error_.assign(message);
                if(!consumer_tg_.empty()) consumer_close();
            });
            ch.resume();
        });
        chn_.onError([this, &ch](const char *message) {
            error_ = message;
            chn_.onReady(nullptr);
            ch.resume();
        });
        ch.suspend();

        if(!error_.empty()) {
            std::string err = std::move(error_);
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to connect RabbitMQ server: %s") % err).str()
                , -1);
        }

        auto i = u.query.find("prefetch");
        if (i == u.query.end()) {
            pf_ = 8;
        }else{
            pf_ = std::min(std::max(std::atoi(i->second.c_str()), 1), 256);
        }
        chn_.setQos(pf_);

        CHECK_AND_SET_FLAG(nolocal, AMQP::nolocal);
        CHECK_AND_SET_FLAG(noack, AMQP::noack);
        CHECK_AND_SET_FLAG(exclusive, AMQP::noack);
        CHECK_AND_SET_FLAG(mandatory, AMQP::mandatory);
        CHECK_AND_SET_FLAG(immediate, AMQP::immediate);

        // 启动心跳
        heartbeat();
    }

    void _client::heartbeat() {
        tm_.expires_after(std::chrono::seconds(45));
        tm_.async_wait([this] (const boost::system::error_code& error) {
            if(error) return;
            con_.heartbeat();
            heartbeat();
        });
    }

    bool _client::has_error() {
        return !error_.empty();
    }

    const std::string& _client::error() {
        return error_;
    }

    void _client::consume(const std::string &queue, coroutine_queue<php::object>& q, coroutine_handler &ch) {
        php::object obj = nullptr;
        const char* err = nullptr;
        chn_.consume(queue)
            .onReceived([this, &q, &obj, &ch](const AMQP::Message &m, std::uint64_t tag, bool redelivered) {
                obj = php::object(php::class_entry<message>::entry());
                message* ptr = static_cast<message*>(php::native(obj));
                ptr->build_ex(m, tag);
                ch.resume(); // ----> 2
            })
            .onSuccess([this, &ch](const std::string &t) {
                consumer_tg_ = t;
                ch.resume(); // ----> 2
            })
            .onError([&err, &ch](const char *message) {
                err = message;
                ch.resume(); // ----> 2
            });
        ch.suspend();
        if(err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to consume RabbitMQ queue: %s") % err).str()
            , -1);
        consumer_ch_ = ch;
        ch.suspend();
        // 非关闭恢复执行, 消息对象一定存在
        while(!consumer_tg_.empty()) {
            q.push(std::move(obj), ch);
            ch.suspend(); // 2 <----
        }
        if(err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to consume RabbitMQ queue: %s") % err).str()
            , -1);
        q.close();
        consumer_ch_.reset();
    }

    void _client::confirm(std::uint64_t tag, coroutine_handler &ch) {
        chn_.ack(tag, 0);
    }

    void _client::reject(std::uint64_t tag, int flags, coroutine_handler &ch) {
        chn_.reject(tag, flags);
    }

    void _client::consumer_close() {
        const char* err = nullptr;
        chn_.cancel(consumer_tg_);
        consumer_tg_.clear();
        // 标记结束后，消费流程队将自行关闭
        if(consumer_ch_) consumer_ch_.resume();
    }

    void _client::consumer_close(coroutine_handler& ch) {
        const char* err = nullptr;
        chn_.cancel(consumer_tg_)
            .onSuccess([&ch] () {
                ch.resume();
            })
            .onError([&err, &ch] (const char* message) {
                err = message;
                ch.resume();
            });
        ch.suspend();
        consumer_tg_.clear();
        if(consumer_ch_) consumer_ch_.resume(); // ----> 2  标记结束后，消费流程队将自行关闭
        if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to close RabbitMQ consumer: %s") % err).str()
            , -1);
    }

    void _client::publish(const std::string &ex, const std::string &rk, const AMQP::Envelope &env, coroutine_handler& ch) {
        /*auto& defer = */chn_.publish(ex, rk, env, fl_);
        // producer_cb(defer, ch);
        if(!error_.empty()) {
            std::string err = std::move(error_);
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to publish to RabbitMQ: %s") % err).str()
                , -1);
        }
    }

    // void _client::producer_cb(AMQP::DeferredPublisher& defer, coroutine_handler& ch) {
    //     producer_ch_ = ch;
    //     if(!producer_cb_) {
    //         producer_cb_ = true;
    //         const char* err = nullptr;
    //         defer.onSuccess([this]() {
    //             if(producer_ch_) producer_ch_.resume();
    //         }).onError([this](const char *message) {
    //             error_ = message;
    //             if(producer_ch_) producer_ch_.resume();
    //         });
    //     }
    //     ch.suspend();
    //     producer_ch_.reset();
    //     if (!error_.empty()) {
    //         std::string err = std::move(error_);
    //         throw php::exception(zend_ce_error,
    //                         (boost::format("failed to publish RabbitMQ message: %1%") % err).str(), -1);
    //     }
    // }

    void _client::publish(const std::string &ex, const std::string &rk, const char *msg, size_t len, coroutine_handler& ch)
    {
        /*auto& defer = */chn_.publish(ex, rk, msg, len, fl_);
        // producer_cb(defer, ch);
        if(!error_.empty()) {
            std::string err = std::move(error_);
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to publish RabbitMQ message: %s") % err).str()
                , -1);
        }
    }
}
