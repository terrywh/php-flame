#include "kafka.h"
#include "message.h"
#include "consumer.h"
#include "_consumer.h"
#include "producer.h"
#include "_producer.h"

namespace flame::kafka
{
    void declare(php::extension_entry &ext)
    {
        ext
			.function<consume>("flame\\kafka\\consume", {
				{"options", php::TYPE::ARRAY},
                {"topics", php::TYPE::ARRAY},
			})
            .function<produce>("flame\\kafka\\produce", {
				{"options", php::TYPE::ARRAY},
                {"topics", php::TYPE::ARRAY},
			});;
		message::declare(ext);
        consumer::declare(ext);
		producer::declare(ext);
    }
    
    php::value consume(php::parameters& params)
    {
        php::array config = params[0];
        php::array topics = params[1];

        php::object obj(php::class_entry<consumer>::entry());
        consumer* ptr = static_cast<consumer*>(php::native(obj));
        if (config.exists("concurrent"))
        {
            ptr->cc_ = std::min(std::max(static_cast<int>(config.get("concurrent")), 1), 256);
            config.erase("concurrent");
        }
        if (!config.exists("log.connection.close")) {
            config.set("log.connection.close", "false");
        }
        ptr->cs_.reset(new _consumer(config, topics));
        coroutine_handler ch {coroutine::current};
        // 订阅
        ptr->cs_->subscribe(ch);
        return std::move(obj);
    }
    php::value produce(php::parameters& params)
    {
        php::array config = params[0];
        php::array topics = params[1];

        php::object obj(php::class_entry<producer>::entry());
        producer* ptr = static_cast<producer*>(php::native(obj));
        if (!config.exists("log.connection.close")) {
            config.set("log.connection.close", "false");
        }
        ptr->pd_.reset(new _producer(config, topics));
        // TODO 优化: 确认首次连接已建立
        return std::move(obj);
    }


    rd_kafka_conf_t* array2conf(const php::array &config) {
        if(config.empty()) return nullptr;
        char err[256];
        rd_kafka_conf_t *conf = rd_kafka_conf_new();
        for (auto i = config.begin(); i != config.end(); ++i) {
            php::string key = i->first.to_string();
            php::string val = i->second.to_string();

            if(RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, key.data(), val.data(), err, sizeof(err)))
                throw php::exception(zend_ce_type_error,
                        (boost::format("unable to set Kafka config: %1%") % err).str(), -1);
        }
        return conf;
    }
    rd_kafka_headers_t* array2hdrs(const php::array& data)
    {
        rd_kafka_headers_t *hdrs = nullptr;
        if (!data.empty() && data.typeof(php::TYPE::ARRAY) && data.size() > 0)
        {
            hdrs = rd_kafka_headers_new(data.size());
            for (auto i = data.begin(); i != data.end(); ++i)
            {
                php::string key = i->first.to_string(),
                            val = i->second.to_string();
                rd_kafka_header_add(hdrs,
                                    key.c_str(), key.size(),
                                    val.c_str(), val.size());
            }
        }
        return hdrs;
    }
    php::array hdrs2array(rd_kafka_headers_t* hdrs)
    {
        if(hdrs == nullptr) return php::array(0);
        php::array header(rd_kafka_header_cnt(hdrs));
        const char *key;
        const void *val;
        std::size_t len;
        for (std::size_t i = 0; rd_kafka_header_get_all(hdrs, i, &key, &val, &len) == RD_KAFKA_RESP_ERR_NO_ERROR; ++i)
        {
            header.set(key, php::string((const char *)val, len));
        }
        return header;
    }
}
