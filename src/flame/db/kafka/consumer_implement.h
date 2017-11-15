#pragma once

#include "../../thread_worker.h"

namespace flame {
	class coroutine;
namespace db {
namespace kafka {
	class consumer;
	class consumer_implement {
	public:
		consumer_implement(consumer* p, php::parameters& params);
		void consume1(php::callable& cb);
		void stop();
		void consume2();
		void commit(php::object& msg);
		void close();
	private:
		consumer*                        consumer_;
		thread_worker                    worker_;
		rd_kafka_t*                      kafka_;
		rd_kafka_topic_partition_list_t* topic_;
		typedef struct consume_request_t {
			coroutine*            co;
			consumer_implement* self;
			php::object          ref;
			php::callable         cb;
			php::value            rv;
			uv_work_t            req;
		} consume_request_t;
		consume_request_t* ctx_;
		typedef struct commit_t {
			coroutine*           co;
			consumer_implement* self;
			php::value          ref;
			rd_kafka_message_t* msg;
			php::value          msg_ref;
			php::value           rv;
			uv_work_t           req;
		} commit_t;
		std::deque<commit_t*> commit_;

		static void error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
		static void consume1_wk(uv_work_t* handle);
		static void consume1_cb(uv_work_t* handle, int status);
		static void consume2_wk(uv_work_t* handle);
		static void consume2_cb(uv_work_t* handle, int status);
		static void commit_wk(uv_work_t* handle);
		static void offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque);
		static void commit_cb(uv_work_t* handle, int status);
		static void close_cb(uv_handle_t* handle);
	};
}
}
}
