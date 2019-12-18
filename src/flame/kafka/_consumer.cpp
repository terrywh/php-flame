#include "../controller.h"
#include "../time/time.h"
#include "../log/logger.h"
#include "_consumer.h"
#include "../../coroutine_queue.h"
#include "kafka.h"
#include "message.h"

namespace flame::kafka {
    static rd_kafka_topic_partition_list_t *array2topics(const php::array &topics) {
        // 目标订阅的 TOPIC
        rd_kafka_topic_partition_list_t *t = rd_kafka_topic_partition_list_new(topics.size());
        for (auto i = topics.begin(); i != topics.end(); ++i)
            rd_kafka_topic_partition_list_add(t, i->second.to_string().c_str(), RD_KAFKA_PARTITION_UA);
        return t;
    }

    _consumer::_consumer(php::array &config, php::array &topics)
    : close_(false) {
        if (!config.exists("bootstrap.servers") && !config.exists("metadata.broker.list"))
            throw php::exception(zend_ce_type_error
                , "Failed to create Kafka consumer: 'bootstrap.servers' required"
                , -1);

        if (!config.exists("group.id"))
            throw php::exception(zend_ce_type_error
                , "Failed to create Kafka consumer: 'group.id' required"
                , -1);

        if (topics.size() == 0)
            throw php::exception(zend_ce_type_error
                , "Failed to create Kafka consumer: target topics missing"
                , -1);

        rd_kafka_conf_t *conf = array2conf(config);
        char err[256];
        rd_kafka_conf_set_opaque(conf, this);
        rd_kafka_conf_set_error_cb(conf, on_error);

        conn_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf, err, sizeof(err));
        if (!conn_)
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to create Kafka consumer: %s") % err).str()/*, 0*/);

        auto r = rd_kafka_poll_set_consumer(conn_);
        if (r != RD_KAFKA_RESP_ERR_NO_ERROR)
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to create Kafka Consumer: %s") % rd_kafka_err2str(r)).str()
                , r);


        tops_ = array2topics(topics);
    }

    _consumer::~_consumer() {
        if (conn_) {
            // rd_kafka_consumer_close(conn_);
            rd_kafka_topic_partition_list_destroy(tops_);
            rd_kafka_destroy(conn_);
        }
    }

    void _consumer::on_error(rd_kafka_t* conn, int error, const char* reason, void* data) {
        _consumer* self = reinterpret_cast<_consumer*>(data);
        if (log::logger::LEVEL_OPT <= log::logger::LEVEL_WARNING)
            log::logger_->stream() << "[" << time::iso() << "] (WARNING) Kafka Consumer " << rd_kafka_err2str((rd_kafka_resp_err_t)error) << ": " << reason << std::endl;
    }

    void _consumer::subscribe(coroutine_handler &ch) {
        rd_kafka_resp_err_t err;
        boost::asio::post(gcontroller->context_y, [this, &err, &ch] ()  {
            err = rd_kafka_subscribe(conn_, tops_);
            ch.resume();
        });
        ch.suspend();
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to subcribe Kafka topics: %s") % rd_kafka_err2str(err)).str()
                , err);
    }
    // TODO 考虑将消费过程迁移置工作线程：目前在主线程运行
    bool _consumer::consume(coroutine_queue<rd_kafka_message_t*>& q, ::coroutine_handler& ch) {
        std::unique_ptr<rd_kafka_message_t, decltype(rd_kafka_message_destroy)*>
            msg {rd_kafka_consumer_poll(conn_, 0), rd_kafka_message_destroy };
        if (!msg) {
            // close_ == true // 消费被停止
            return false;
        }
        else if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            return false;
        }
        else if (msg->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to consume Kafka message: %s / %s")
                    % rd_kafka_err2str(msg->err)
                    % rd_kafka_message_errstr(msg.get())).str()
                , msg->err);
        }
        else {
            q.push(msg.release(), ch);
            return true;
        }
    }

    void _consumer::commit(const php::object& obj, coroutine_handler& ch) {
        rd_kafka_resp_err_t err;
        boost::asio::post(gcontroller->context_y, [this, &err, msg = static_cast<message*>(php::native(obj))->msg_, &ch]() {
            err = rd_kafka_commit_message(conn_, msg, 0);
            ch.resume();
        });
        ch.suspend();
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to commit Kafka message: %s") % err % rd_kafka_err2str(err)).str()
                , err);
    }

    void _consumer::close() {
        close_ = true;
        // 由于不再协程上下文暂停，需要共享延长生命周期
        boost::asio::post(gcontroller->context_y, [self = shared_from_this(), this] () {
            rd_kafka_consumer_close(conn_);
        });
    }

} // namespace flame::kafka
