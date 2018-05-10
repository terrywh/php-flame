#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	class consumer;
	class producer;
	class client_implement {
	public:
		client_implement(bool is_producer);
	private:
		thread_worker         worker_;
		amqp_connection_state_t conn_;
		uv_timer_t             timer_;
		std::shared_ptr<php_url> parse_url(const php::string& url);
		void connect(std::shared_ptr<php_url> url);
		void subscribe(const php::string& q, uint16_t prefetch);
		void destroy(bool close_channel = true);
		void reset_timer();

		static void produce_wk(uv_work_t* req);
		static void flush_wk(uv_work_t* req);
		static void consume_wk(uv_work_t* req);
		static void confirm_message_wk(uv_work_t* req);
		static void reject_message_wk(uv_work_t* req);
		static void destroy_message_wk(uv_work_t* req);
		static void destroy_message_cb(uv_work_t* req, int status);
		static void destroy_wk(uv_work_t* req);
		static void destroy_cb(uv_work_t* req, int status);
		static void timer_wk(uv_timer_t* tm);
		static void error_cb(uv_work_t* req, int status);

		union {
			consumer* consumer_;
			producer* producer_;
		};
		bool is_producer;
		friend class consumer;
		friend class producer;
		friend class message;
	};
	typedef struct client_request_t {
		coroutine*            co;
		client_implement*   self;
		php::value           msg;
		php::value           key;
		php::value            rv;
		uv_work_t            req;
	} client_request_t;
}
}
}
