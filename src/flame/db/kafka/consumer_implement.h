#pragma once

#include "../../thread_worker.h"

namespace flame {
	class coroutine;
namespace db {
namespace kafka {
	class consumer;
	class consumer_implement;
	typedef struct consumer_request_t {
		coroutine*            co;
		consumer_implement* self;
		php::value            rv;
		php::object          msg;
		uv_work_t            req;
	} consumer_request_t;
	
	class consumer_implement {
	public:
		consumer_implement(consumer* p, php::parameters& params);
	private:
		consumer*                        consumer_;
		thread_worker                    worker_;
		rd_kafka_t*                      kafka_;
		rd_kafka_topic_partition_list_t* topic_;
		consumer_request_t*      ctx_; // 用于 commit
		static void error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
		static void consume_wk(uv_work_t* handle);
		static void commit_wk(uv_work_t* handle);
		static void offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque);
		static void destroy_wk(uv_work_t* handle);
		static void destroy_cb(uv_work_t* handle, int status);
		static void destroy_msg_wk(uv_work_t* handle);
		
		friend class consumer;
		friend class message;
	};
}
}
}
