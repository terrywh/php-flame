#include "../controller.h"
#include "_consumer.h"
#include "kafka.h"
#include "message.h"
#include "../coroutine_queue.h"

namespace flame::kafka
{
    static rd_kafka_topic_partition_list_t *array2topics(const php::array &topics)
    { // 目标订阅的 TOPIC
        rd_kafka_topic_partition_list_t *t = rd_kafka_topic_partition_list_new(topics.size());
        for (auto i = topics.begin(); i != topics.end(); ++i)
        {
            rd_kafka_topic_partition_list_add(t, i->second.to_string().c_str(), RD_KAFKA_PARTITION_UA);
        }
        return t;
    }
    _consumer::_consumer(php::array &config, php::array &topics)
    : close_(false)
    {
        if (!config.exists("bootstrap.servers") && !config.exists("metadata.broker.list"))
        {
            throw php::exception(zend_ce_type_error, "kafka conf 'bootstrap.servers' is required");
        }
        if (!config.exists("group.id"))
        {
            throw php::exception(zend_ce_type_error, "kafka conf 'group.id' is required");
        }
        rd_kafka_conf_t *conf = array2conf(config);
        char err[256];

        conn_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf, err, sizeof(err));
        if (!conn_)
        {
            throw php::exception(zend_ce_exception,
                                (boost::format("failed to create Kafka Consumer: %1%") % err).str(), -1);
        }
        auto r = rd_kafka_poll_set_consumer(conn_);
        if (r != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            throw php::exception(zend_ce_exception,
                                 (boost::format("failed to create Kafka Consumer: (%1%) %2%") % r % rd_kafka_err2str(r)).str(), r);
        }
        if (topics.size() == 0)
        {
            throw php::exception(zend_ce_type_error, "failed to create Kafka Consumer: target topics missing", -1);
        }
        tops_ = array2topics(topics);
    }
    _consumer::~_consumer()
    {
        if(conn_) {
            // rd_kafka_consumer_close(conn_);
            rd_kafka_topic_partition_list_destroy(tops_);
            rd_kafka_destroy(conn_);
        }
    }
    void _consumer::subscribe(coroutine_handler &ch)
    {
        rd_kafka_resp_err_t err;
        boost::asio::post(gcontroller->context_y, [this, &err, &ch] ()
        {
            err = rd_kafka_subscribe(conn_, tops_);
            ch.resume();
        });
        ch.suspend();
        if(err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            throw php::exception(zend_ce_exception,
                (boost::format("failed to subcribe Kafka topics: (%1%) %2%") % err % rd_kafka_err2str(err)).str(),
                err);
        }
    }
    void _consumer::consume(coroutine_queue<php::object>& q, coroutine_handler& ch)
    {
        rd_kafka_resp_err_t err;
        rd_kafka_message_t* msg = nullptr;
POLL_MESSAGE:
        // 由于 poll 动作本身阻塞, 为防止所有工作线被占用, 这里绑定到 strd_ 执行
        // (理论上仅占用一个协程)
        boost::asio::post(gcontroller->context_y, [this, &err, &msg, &ch]()
        {
            msg = rd_kafka_consumer_poll(conn_, 500);
            if(!msg) err = RD_KAFKA_RESP_ERR__PARTITION_EOF;
            else if (msg->err)
            {
                err = msg->err;
                rd_kafka_message_destroy(msg);
            }
            else err = RD_KAFKA_RESP_ERR_NO_ERROR;
            ch.resume();
        });
        ch.suspend();
        if(err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
        {
            if(close_)
            {
                q.close(); // 关闭队列使对应消费协程自行退出
                return;
            } // 消费已被关闭
            else goto POLL_MESSAGE;
        }
        else if(err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            throw php::exception(zend_ce_exception,
                (boost::format("failed to consume Kafka message: (%1%) %2%") % err % rd_kafka_err2str(err)).str(),
                err);
        }
        else
        {
            php::object obj(php::class_entry<message>::entry());
            message* ptr = static_cast<message*>(php::native(obj));
            ptr->build_ex(msg); // msg 交由 message 对象管理
            q.push(std::move(obj), ch);
            goto POLL_MESSAGE;
        }
    }
    void _consumer::commit(const php::object& obj, coroutine_handler& ch)
    {
        rd_kafka_resp_err_t err;
        rd_kafka_message_t *msg = static_cast<message*>(php::native(obj))->msg_;
        boost::asio::post(gcontroller->context_y, [this, &err, &msg, &ch]()
        {
            err = rd_kafka_commit_message(conn_, msg, 0);
            ch.resume();
        });
        ch.suspend();
        if(err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            throw php::exception(zend_ce_exception,
                                 (boost::format("failed to commit Kafka message: (%1%) %2%") % err % rd_kafka_err2str(err)).str(),
                                 err);
        }
    }
    void _consumer::close(coroutine_handler& ch)
    {
        rd_kafka_resp_err_t err;
        boost::asio::post(gcontroller->context_y, [this, &err, &ch] () {
            err = rd_kafka_consumer_close(conn_);
            ch.resume();
        });
        ch.suspend();
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            throw php::exception(zend_ce_exception,
                                 (boost::format("failed to close Kafka consumer: (%1%) %2%") % err % rd_kafka_err2str(err)).str(),
                                 err);
        }
        close_ = true;
    }

} // namespace flame::kafka
