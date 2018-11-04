#include "../vendor.h"
#include "../controller.h"
#include "../coroutine.h"
#include "_producer.h"

namespace flame {
namespace kafka {
    _producer::_producer(const php::array& conf, const php::array& topics) {
        char error[256];
        int  basic = 0;
        rd_kafka_conf_t* conf_ = rd_kafka_conf_new();
        // 配置
        for(auto i=conf.begin(); i!=conf.end(); ++i) {
            php::string key = i->first.to_string();
            php::string val = i->second.to_string();
            if(rd_kafka_conf_set(conf_, key.c_str(), val.c_str(), error, sizeof(error)) != RD_KAFKA_CONF_OK) {
                throw php::exception(zend_ce_type_error,
                    (boost::format("failed to set kafka consumer config: %s") % error).str());
            }
            if(std::strncmp(key.c_str(), "bootstrap.servers", 17) == 0
                || std::strncmp(key.c_str(), "metadata.broker.list", 20) == 0) {
                basic |= 0x01;
            }
        }
        if( !(basic & 0x01) ) { // 缺少必要参数
            throw php::exception(zend_ce_type_error, "'bootstrap.servers' is required");
        }
        // 默认
        // rd_kafka_topic_conf_t* tconf = rd_kafka_topic_conf_new();
        // rd_kafka_topic_conf_set_partitioner_cb(tconf, rd_kafka_msg_partitioner_consistent_random);
        // rd_kafka_conf_set_default_topic_conf(conf, tconf);

        conn_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, error, sizeof(error));
        if(conn_ == nullptr) {
            throw php::exception(zend_ce_type_error, (boost::format("failed to create kafka consumer: %s") % error).str());
        }
        for(auto i=topics.begin(); i!=topics.end(); ++i) {
            php::string topic = i->second.to_string();
            tops_[topic] = rd_kafka_topic_new(conn_, topic.c_str(), nullptr);
        }
    }
    _producer::~_producer() {
        for(auto i=tops_.begin(); i!=tops_.end(); ++i) {
            rd_kafka_topic_destroy(i->second);
        }
        rd_kafka_destroy(conn_);
    }

    _producer& _producer::exec(worker_fn_t&& wk, master_fn_t&& fn) {
        auto ptr = this->shared_from_this();
        // 避免在工作线程中对 wk 捕获的 PHP 对象进行拷贝释放
        auto wk_ = std::make_shared<worker_fn_t>(std::move(wk));
        auto fn_ = std::make_shared<master_fn_t>(std::move(fn));
        boost::asio::post(controller_->context_ex, [this, wk_, fn_, ptr] () {
            rd_kafka_resp_err_t e = (*wk_)(conn_);
            boost::asio::post(context, [this, wk_, fn_, e, ptr] () {
                (*fn_)(conn_, e);
            });
        }); // 访问当前已持有的连接
        return *this;
    }
    rd_kafka_resp_err_t _producer::publish_wk(php::string topic, php::string key, php::string payload, rd_kafka_headers_t* hdrs) {
        if(tops_.count(topic) > 0) {
            return rd_kafka_producev(conn_,
                RD_KAFKA_V_TOPIC(topic.c_str()),
                RD_KAFKA_V_KEY(key.c_str(), key.size()),
                RD_KAFKA_V_PARTITION(RD_KAFKA_PARTITION_UA),
                RD_KAFKA_V_HEADERS(hdrs),
                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                RD_KAFKA_V_VALUE((void*)payload.c_str(), payload.size()),
                RD_KAFKA_V_END);
        }else{
            return RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC;
        }
    }
}
}
