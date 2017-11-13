#pragma once

namespace flame {
	class coroutine;
namespace db {
namespace kafka {
	class consumer;
	class consumer_implement {
	public:
		consumer_implement(consumer* p, php::parameters& params);
		void consume(php::callable& cb);
		void consume();
		void commit(php::object& msg);
		void close();
	private:
		consumer*                        consumer_;
		rd_kafka_t*                      kafka_;
		rd_kafka_conf_t*                 gconf_;
		rd_kafka_topic_partition_list_t* topic_;
		uv_check_t                       check_;
		void*                            context_;
		typedef struct commit_t {
			coroutine*   co;
			php::object ref;
			php::object msg;
		} commit_t;
		std::map<rd_kafka_message_t*, commit_t> commits_;
		static void error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
		static void consume_cb(uv_check_t* handle);
		static void commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque);
		static void close_cb(uv_handle_t* handle);
	};
}
}
}
