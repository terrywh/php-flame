#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	class consumer;
	class producer;
	class client_implement {
	private:
		thread_worker         worker_;
		amqp_connection_state_t conn_;

		std::shared_ptr<php_url> parse_url(const php::string& url);
		void connect(std::shared_ptr<php_url> url);
		void subscribe(const php::string& q);
		void destroy(bool close_channel = true);

		static void produce_wk(uv_work_t* req);
		static void flush_wk(uv_work_t* req);
		static void consume_wk(uv_work_t* req);
		static void confirm_envelope_wk(uv_work_t* req);
		static void reject_envelope_wk(uv_work_t* req);
		static void destroy_envelope_wk(uv_work_t* req);
		static void destroy_wk(uv_work_t* req);
		static void destroy_cb(uv_work_t* req, int status);

		static void error_cb(uv_work_t* req, int status);

		union {
			consumer* consumer_;
			producer* producer_;
		};
		friend class consumer;
		friend class producer;
		friend class message;
	};
	typedef struct client_request_t {
		coroutine*            co;
		client_implement*   self;
		php::string          msg;
		php::string          key;
		php::value            rv;
		uv_work_t            req;
	} client_request_t;
}
}
}
