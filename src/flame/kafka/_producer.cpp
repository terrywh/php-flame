#include "../controller.h"
#include "../time/time.h"
#include "../log/logger.h"
#include "_producer.h"
#include "kafka.h"

namespace flame::kafka {

    _producer::_producer(php::array& config, php::array& topics)
    : poll_(gcontroller->context_y)
    , close_(false) {
        if (!config.exists("bootstrap.servers") && !config.exists("metadata.broker.list"))
            throw php::exception(zend_ce_type_error
                , "Failed to create Kafka producer: 'bootstrap.servers' required"
                , -1);
        if (topics.size() == 0)
            throw php::exception(zend_ce_type_error
                , "Failed to create Kafka producer: target topics missing"
                , -1);

        rd_kafka_conf_t *conf = array2conf(config);
        rd_kafka_conf_set_opaque(conf, this);
        rd_kafka_conf_set_error_cb(conf, on_error);

        char err[256];
        conn_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, err, sizeof(err));
        if (!conn_)
            throw php::exception(zend_ce_type_error
                , (boost::format("Failed to create Kafka Consumer: %s") % err).str()
                , -1);

        for (auto i = topics.begin(); i != topics.end(); ++i) {
            php::string topic = i->second.to_string();
            tops_[topic] = rd_kafka_topic_new(conn_, topic.c_str(), nullptr);
        }
    }

    _producer::~_producer() {
        if (!conn_) return;
        for(auto i=tops_.begin(); i!=tops_.end(); ++i) {
            rd_kafka_topic_destroy(i->second);
        }
        rd_kafka_destroy(conn_);
    }

    void _producer::on_error(rd_kafka_t* conn, int error, const char* reason, void* data) {
        _producer* self = reinterpret_cast<_producer*>(data);
        if (log::logger::LEVEL_OPT <= log::logger::LEVEL_WARNING)
            log::logger_->stream() << "[" << time::iso() << "] (WARNING) Kafka Producer " << rd_kafka_err2str((rd_kafka_resp_err_t)error) << ": " << reason << "\n";
    }

    void _producer::start() {
        poll();
    }

    void _producer::poll(int expire) {
        poll_.expires_after(std::chrono::milliseconds(expire));
        poll_.async_wait([this, self = shared_from_this()] (const boost::system::error_code& error) {
            if (error || close_) return;
            auto begin = std::chrono::steady_clock::now();
            int r = rd_kafka_poll(conn_, 40); // 防止对工作线程占用时间过长
            if (close_) return;
            auto end = std::chrono::steady_clock::now();
            // 计算实际 poll 占用时间，稳定间隔
            int e = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            // 无消息时放慢 poll 以减少对工作线程的占用
            if (r > 0) poll(std::max(40 - e, 1));
            else poll(std::max(1600 - e, 1));
        });
    }

    void _producer::close(coroutine_handler& ch) {
        if (!conn_) return;
        close_ = true;
        poll_.cancel();
        boost::asio::post(gcontroller->context_y, [this, &ch] {
            rd_kafka_yield(conn_);
            rd_kafka_flush(conn_, 10000);
            ch.resume();
        });
        ch.suspend();
    }

    void _producer::publish(const php::string &topic, const php::string &key
        , const php::string &payload, const php::array &headers, coroutine_handler &ch) {

        rd_kafka_resp_err_t err = RD_KAFKA_RESP_ERR_NO_ERROR;
        rd_kafka_headers_t* hdrs = array2hdrs(headers);
        boost::asio::post(gcontroller->context_y, [this, &err, &topic, &key, &payload, &hdrs, &ch]() {
            if (key.size() > 0) {
                err = rd_kafka_producev(conn_,
                    RD_KAFKA_V_TOPIC(topic.c_str()),
                    RD_KAFKA_V_KEY(key.c_str(), key.size()),
                    RD_KAFKA_V_PARTITION(RD_KAFKA_PARTITION_UA),
                    RD_KAFKA_V_HEADERS(hdrs),
                    RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                    RD_KAFKA_V_VALUE((void *)payload.c_str(), payload.size()),
                    RD_KAFKA_V_END);
            }
            else { // 无 KEY 时默认 partitioner 会随机分配
                err = rd_kafka_producev(conn_,
                    RD_KAFKA_V_TOPIC(topic.c_str()),
                    RD_KAFKA_V_PARTITION(RD_KAFKA_PARTITION_UA),
                    RD_KAFKA_V_HEADERS(hdrs),
                    RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                    RD_KAFKA_V_VALUE((void *)payload.c_str(), payload.size()),
                    RD_KAFKA_V_END);
            }
            ch.resume();
        });
        ch.suspend();
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            if (hdrs) rd_kafka_headers_destroy(hdrs);  // 发生错误时, 需要手动销毁
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to publish Kafka message: %s") % rd_kafka_err2str(err)).str()
                , err);
        }
    }

    void _producer::flush(coroutine_handler& ch) {
        if (!conn_) return;
        poll_.cancel();
        rd_kafka_yield(conn_); // 停止当前的 poll
        rd_kafka_flush(conn_, 10000);
        poll(); // 重新启动 poll 流程
    }
} // namespace flame::kafka
