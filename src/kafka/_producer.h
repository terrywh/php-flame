#pragma

namespace flame {
namespace kafka {
    class _producer: public std::enable_shared_from_this<_producer> {
    public:
        typedef std::function<rd_kafka_resp_err_t (rd_kafka_t* c)> worker_fn_t;
        typedef std::function<void (rd_kafka_t* c, rd_kafka_resp_err_t  e)> master_fn_t;
        _producer(const php::array& config, const php::array& topics);
        ~_producer();
        _producer& exec(worker_fn_t&& wk, master_fn_t&& fn);
        rd_kafka_resp_err_t publish_wk(php::string topic, php::string key, php::string payload, rd_kafka_headers_t* hdrs);
    private:
        rd_kafka_t*                              conn_;
        std::map<php::string, rd_kafka_topic_t*> tops_;
        friend class message;
    };
}
}
