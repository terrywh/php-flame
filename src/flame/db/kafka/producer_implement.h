#pragma once

#include "../../thread_worker.h"

namespace flame {
namespace db {
namespace kafka {
	class producer;
	class producer_implement {
	public:
		producer_implement(producer* p, php::parameters& params);
		void produce(const php::string& val, const php::string& key);
		void close();
	private:
		producer*              producer_;
		thread_worker          worker_;
		rd_kafka_t*            kafka_;
		rd_kafka_topic_t*      topic_;

		php::callable  partitioner_fn;
		void           partitioner_cb(rd_kafka_topic_conf_t* tconf_, php::value& cb);
		static int32_t partitioner_cb(
				const rd_kafka_topic_t *rkt, const void *keydata, size_t keylen,
				int32_t partition_cnt, void *rkt_opaque, void *msg_opaque);
		static void error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
		// static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t * rkmessage, void *opaque);
		
		static void produce_wk(uv_work_t* handle);
		static void   flush_wk(uv_work_t* req);
		static void  destroy_wk(uv_work_t* req);
		static void  destroy_cb(uv_work_t* req, int status);
		friend class producer;
	};
	
	typedef struct producer_request_t {
		coroutine*            co;
		producer_implement* self;
		php::value            rv;
		php::string          val;
		php::string          key;
		uv_work_t            req;
	} producer_request_t;
}
}
}
