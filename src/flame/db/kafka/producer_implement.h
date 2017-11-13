#pragma once

namespace flame {
namespace db {
namespace kafka {
	class producer;
	class producer_implement {
	public:
		producer_implement(producer* p, php::parameters& params);

		void partitioner(php::value& cb);
		void produce(php::string& payload, php::string& key);
		void produce(php::string& payload);
		void consume(php::callable& cb);
		void consume();
		void close();
	private:
		producer*              producer_;
		rd_kafka_t*            kafka_;
		rd_kafka_topic_t*      topic_;
		rd_kafka_conf_t*       gconf_;
		rd_kafka_topic_conf_t* tconf_;

		php::callable  partitioner_fn;
		static int32_t partitioner_cb (
				const rd_kafka_topic_t *rkt, const void *keydata, size_t keylen,
				int32_t partition_cnt, void *rkt_opaque, void *msg_opaque);
		uv_check_t             check_;

		static void error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
		static void produce_cb(uv_check_t* handle);
		static void produce_cb(rd_kafka_t *rk, const rd_kafka_message_t * rkmessage, void *opaque);
		static void close_cb(uv_handle_t* handle);
	};
}
}
}
