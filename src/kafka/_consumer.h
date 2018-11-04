#pragma once

namespace flame {
namespace kafka {

    class _consumer: public std::enable_shared_from_this<_consumer> {
    public:
        typedef std::function<rd_kafka_message_t* (rd_kafka_t* conn, rd_kafka_resp_err_t& e)> worker_fn_t;
        typedef std::function<void (rd_kafka_t* conn, rd_kafka_message_t* msg, rd_kafka_resp_err_t  e)> master_fn_t;
        _consumer(const php::array& config, const php::array& topics);
        ~_consumer();
        _consumer& exec(worker_fn_t&& wk, master_fn_t&& fn);
        rd_kafka_resp_err_t subscribe_wk();
    private:
        rd_kafka_topic_partition_list_t* tops_;
        rd_kafka_t*                      conn_;
        // static void rebalance_cb(rd_kafka_t* conn, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t * part, void* data);

        friend php::value consume(php::parameters& params);
        friend class message;
    };

}
}
