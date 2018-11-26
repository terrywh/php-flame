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

#define CHECK_AND_SET_FLAG(xflag, yflag)                  \
    for (auto i = u.query.find(#xflag); i != u.query.end();)    \
    {                                                           \
        const char *str = i->second.c_str();                    \
        if (str2bool(str))                                      \
        {                                                       \
            fl_ |= yflag;                                      \
        }                                                       \
    }

    _client::_client(url u, coroutine_handler& ch)
        : hd_(gcontroller->context_x), cc_(&hd_, AMQP::Address(u.str(true, false).c_str(), u.str().size())), ch_(&cc_), consumer_closed_(false)
    {
        const char *err = nullptr;
        ch_.onReady([this, &ch]() {
            ch_.onError(nullptr);
            ch.resume();
        });
        ch_.onError([this, &err, &ch](const char *message) {
            err = message;
            ch_.onReady(nullptr);
            ch.resume();
        });
        ch.suspend();

        auto i = u.query.find("prefetch");
        if(i == u.query.end()) {
            pf_ = 8;
        }else{
            pf_ = std::min(std::max(std::atoi(i->second.c_str()), 1), 1024);
        }
        ch_.setQos(pf_);

        CHECK_AND_SET_FLAG(nolocal, AMQP::nolocal);
        CHECK_AND_SET_FLAG(noack, AMQP::noack);
        CHECK_AND_SET_FLAG(exclusive, AMQP::noack);
        CHECK_AND_SET_FLAG(mandatory, AMQP::mandatory);
        CHECK_AND_SET_FLAG(immediate, AMQP::immediate);
        
        if(err) {
            throw php::exception(zend_ce_error,
                (boost::format("failed to connect RabbitMQ server: %1%") % err).str(), -1);
        }
    }

    void _client::connect(coroutine_handler& ch)
    {
        
    }
    void _client::consume(const std::string &queue, coroutine_queue<php::object>& q, coroutine_handler &ch)
    {
        php::object obj = nullptr;
        const char* err = nullptr;
        ch_.consume(queue)
            .onReceived([this, &q, &obj, &ch](const AMQP::Message &m, std::uint64_t tag, bool redelivered)
            {
                obj = php::object(php::class_entry<message>::entry());
                message* ptr = static_cast<message*>(php::native(obj));
                ptr->build_ex(m, tag);
                ch.resume();
            })
            .onSuccess([this, &ch](const std::string &t)
            {
                tag_ = t;
                ch.resume();
            })
            .onError([&err, &ch](const char *message)
            {
                err = message;
                ch.resume();
            });
        ch.suspend();
        if(err) {
            throw php::exception(zend_ce_error,
                (boost::format("failed to consume RabbitMQ message: %1%") % err).str(), -1);
        }
        consumer_ch_ = ch;
        ch.suspend();
        // 非关闭恢复执行, 消息对象一定存在
        while(!consumer_closed_/* && !obj.empty()*/)
        {
            q.push(std::move(obj), ch);
            ch.suspend();
        }
        q.close();
        consumer_ch_.reset();
    }
    void _client::confirm(std::uint64_t tag, coroutine_handler &ch)
    {
        ch_.ack(tag, 0);
    }
    void _client::reject(std::uint64_t tag, int flags, coroutine_handler &ch)
    {
        ch_.reject(tag, flags);
    }
    void _client::close_consumer(coroutine_handler& ch)
    {
        const char* err = nullptr;
        ch_.cancel(tag_)
            .onSuccess([&ch] () {
                ch.resume();
            })
            .onError([&err, &ch] (const char* message)
            {
                err = message;
                ch.resume();
            });
        ch.suspend();
        // TODO 关闭消费流程队列
        if (err)
        {
            throw php::exception(zend_ce_error,
                                 (boost::format("failed to close RabbitMQ consumer: %1%") % err).str(), -1);
        }
        consumer_closed_ = true;
        if(consumer_ch_) consumer_ch_.resume();
    }
    void _client::publish(const std::string &ex, const std::string &rk, const AMQP::Envelope &env, coroutine_handler& ch)
    {
        const char* err = nullptr;
        ch_.publish(ex, rk, env, fl_);
        // TODO 目前的流程, 暂时未实现消息发送的确认逻辑
        //     .onSuccess([&ch] ()
        //     {
        //         ch.resume();
        //     })
        //     .onError([&err, &ch] (const char* message)
        //     {
        //         err = message;
        //         ch.resume();
        //     });
        // ch.suspend();
        // if(err) {
        //     throw php::exception(zend_ce_error,
        //                          (boost::format("failed to publish RabbitMQ message: %1%") % err).str(), -1);
        // }
    }
    void _client::publish(const std::string &ex, const std::string &rk, const char *msg, size_t len, coroutine_handler& ch)
    {
        const char *err = nullptr;
        ch_.publish(ex, rk, msg, len, fl_);
        // TODO 目前的流程, 暂时未实现消息发送的确认逻辑
            // .onSuccess([&ch]() {
            //     std::cout << "success\n";
            //     ch.resume();
            // })
            // .onError([&err, &ch](const char *message) {
            //     std::cout << "error\n";
            //     err = message;
            //     ch.resume();
            // });
        // ch.suspend();
        // if (err)
        // {
        //     throw php::exception(zend_ce_error,
        //                          (boost::format("failed to publish RabbitMQ message: %1%") % err).str(), -1);
        // }
    }
}
