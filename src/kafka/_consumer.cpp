#include "../vendor.h"
#include "../controller.h"
#include "../coroutine.h"
#include "_consumer.h"

namespace flame {
namespace kafka {

    static rd_kafka_topic_partition_list_t* array2topics(const php::array& names) { // 目标订阅的 TOPIC
        rd_kafka_topic_partition_list_t* t = rd_kafka_topic_partition_list_new(names.size());
        for(auto i=names.begin(); i!=names.end(); ++i) {
            rd_kafka_topic_partition_list_add(t, i->second.to_string().c_str(), RD_KAFKA_PARTITION_UA);
        }
        return t;
    }

    _consumer::_consumer(const php::array& conf, const php::array& topics) {
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
            if(std::strncmp(key.c_str(), "group.id", 8) == 0) {
                basic |= 0x02;
            }else if(std::strncmp(key.c_str(), "bootstrap.servers", 17) == 0
                || std::strncmp(key.c_str(), "metadata.broker.list", 20) == 0) {
                basic |= 0x01;
            }
        }
        if( !(basic & 0x01) ) { // 缺少必要参数
            throw php::exception(zend_ce_type_error, "'bootstrap.servers' is required");
        }
        if( !(basic & 0x02) ) {// 缺少必要参数
            throw php::exception(zend_ce_type_error, "'group.id' is required");
        }
        // rd_kafka_conf_set_rebalance_cb(conf_, _consumer::rebalance_cb);
        // 连接
        conn_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, error, sizeof(error));
        if(conn_ == nullptr) {
            throw php::exception(zend_ce_type_error, (boost::format("failed to create kafka consumer: %s") % error).str());
        }
        rd_kafka_resp_err_t r = rd_kafka_poll_set_consumer(conn_);
        if(r != RD_KAFKA_RESP_ERR_NO_ERROR) {
            throw php::exception(zend_ce_type_error, (boost::format("failed to create kafka consumer: %d") % r).str());
        }
        //
        tops_ = array2topics(topics);
    }
    _consumer::~_consumer() {
        rd_kafka_consumer_close(conn_);
        rd_kafka_topic_partition_list_destroy(tops_);
        rd_kafka_destroy(conn_);
    }

    rd_kafka_resp_err_t _consumer::subscribe_wk() {
        return rd_kafka_subscribe(conn_, tops_);
    }

    // void _consumer::rebalance_cb(rd_kafka_t* conn, rd_kafka_resp_err_t err,
    //                             rd_kafka_topic_partition_list_t * parts,
    //                             void* data) {
    //     switch (err) {
    // 	case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
    // 		fprintf(stderr, "assigned:\n");
    // 		// print_partition_list(stderr, partitions);
    // 		rd_kafka_assign(conn, parts);
    // 		// wait_eof += partitions->cnt;
    // 		break;
    //
    // 	case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
    // 		fprintf(stderr, "revoked:\n");
    // 		// print_partition_list(stderr, partitions);
    // 		rd_kafka_assign(conn, nullptr);
    // 		// wait_eof = 0;
    // 		break;
    //
    // 	default:
    // 		fprintf(stderr, "failed: %s\n", rd_kafka_err2str(err));
    //         rd_kafka_assign(conn, nullptr);
    // 		break;
    // 	}
    // }
    _consumer& _consumer::exec(worker_fn_t&& wk, master_fn_t&& fn) {
        auto ptr = this->shared_from_this();
        // 避免在工作线程中对 wk 捕获的 PHP 对象进行拷贝释放
        auto wk_ = std::make_shared<worker_fn_t>(std::move(wk));
        auto fn_ = std::make_shared<master_fn_t>(std::move(fn));
        boost::asio::post(controller_->context_ex, [this, wk_, fn_, ptr] () {
            rd_kafka_resp_err_t e = RD_KAFKA_RESP_ERR_NO_ERROR;
            rd_kafka_message_t* m = (*wk_)(conn_, e);
            boost::asio::post(context, [this, wk_, fn_, m, e, ptr] () {
                (*fn_)(conn_, m, e);
            });
        });
        return *this;
    }
}
}
