#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "../../time/time.h"
#include "table.h"
#include "message.h"
#include "consumer.h"
#include "producer.h"
#include "client_implement.h"

namespace flame {
namespace db {
namespace rabbitmq {

	client_implement::client_implement(bool is)
	:is_producer(is) {
		uv_timer_init(&worker_.loop, &timer_);
		timer_.data = this;
	}
	std::shared_ptr<php_url> client_implement::parse_url(const php::string& url) {
		std::shared_ptr<php_url> url_ = php::parse_url(url.c_str(), url.length());
		if(strncasecmp(url_->scheme, "amqp", 4) != 0) {
			throw php::exception(
				amqp_error_string2(AMQP_STATUS_BAD_URL),
				AMQP_STATUS_BAD_URL);
		}
		return url_;
	}
	void client_implement::reset_timer() {
		uv_timer_stop(&timer_);
		uv_timer_start(&timer_, timer_wk, 5000, 0);
	}
	void client_implement::connect(std::shared_ptr<php_url> url_) {
		conn_               = amqp_new_connection();
		amqp_socket_t* sock = amqp_tcp_socket_new(conn_);
		
		int error = amqp_socket_open(sock, url_->host, url_->port);
		if(error != AMQP_STATUS_OK) {
			amqp_destroy_connection(conn_);
			throw php::exception(amqp_error_string2(error), error);
		}
		amqp_rpc_reply_t reply = amqp_login(conn_, url_->path + 1,
			AMQP_DEFAULT_MAX_CHANNELS, AMQP_DEFAULT_FRAME_SIZE, 30,
			AMQP_SASL_METHOD_PLAIN, url_->user, url_->pass);
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			destroy(false);
			throw php::exception(
				amqp_error_string2(reply.library_error), reply.library_error);
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			destroy(false);
			throw php::exception("rabbitmq server failed");
		}
		amqp_channel_open(conn_, 1);
		reply = amqp_get_rpc_reply(conn_);
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			destroy(false);
			throw php::exception(
				amqp_error_string2(reply.library_error), reply.library_error);
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			destroy(false);
			throw php::exception("rabbitmq server failed");
		}
		reset_timer();
	}
	void client_implement::destroy(bool close_channel) {
		if(conn_ == nullptr) return;
		if(close_channel) amqp_channel_close(conn_, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn_);
		uv_close((uv_handle_t*)&timer_, nullptr);
		conn_ = nullptr;
	}
	void client_implement::subscribe(const php::string& q, uint16_t prefetch) {
		amqp_basic_qos(conn_, 1, 0, prefetch, false);
		amqp_bytes_t queue {q.length(), (void*)q.c_str()};
		
		amqp_basic_consume(conn_, 1, queue, amqp_empty_bytes,
			consumer_->opt_no_local,
			consumer_->opt_no_ack,
			consumer_->opt_exclusive,
			consumer_->opt_arguments);
		reset_timer();
	}
	void client_implement::produce_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->conn_ == nullptr) {
			ctx->rv = -2;
			return;
		}
		amqp_bytes_t   abp_body;
		amqp_bytes_t   abp_rkey;
		
		// 在这里声明 headers 保证在执行 publish 时内存有效
		table headers;
		
		abp_body.len   = static_cast<php::string&>(ctx->msg).length();
		abp_body.bytes = static_cast<php::string&>(ctx->msg).data();
		
		if(ctx->key.is_string()) {
			abp_rkey.len   = static_cast<php::string&>(ctx->key).length();
			abp_rkey.bytes = static_cast<php::string&>(ctx->key).data();
		}else{
			abp_rkey = amqp_empty_bytes;
		}
		amqp_basic_properties_t abp_properties;
		std::memset(&abp_properties, 0, sizeof(amqp_basic_properties_t));
		php::array& options = static_cast<php::array&>(ctx->rv);
		if(options.is_array()) {
			php::value opt = options.at("content_type",12);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.content_type.len   = str.length();
				abp_properties.content_type.bytes = str.data();
			}
			opt = options.at("content_encoding",12);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.content_encoding.len   = str.length();
				abp_properties.content_encoding.bytes = str.data();
			}
			opt = options.at("headers", 7);
			if(opt.is_array()) {
				abp_properties._flags |= AMQP_BASIC_HEADERS_FLAG;
				headers.assign(opt);
				headers.fill(&abp_properties.headers);
			}
			opt = options.at("delivery_mode",13);
			if(opt.is_long()) {
				abp_properties._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
				abp_properties.delivery_mode = static_cast<int>(opt);
			}
			opt = options.at("priority",8);
			if(opt.is_long()) {
				abp_properties._flags |= AMQP_BASIC_PRIORITY_FLAG;
				abp_properties.priority = static_cast<int>(opt);
			}
			opt = options.at("correlation_id",14);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.correlation_id.len   = str.length();
				abp_properties.correlation_id.bytes = str.data();
			}
			opt = options.at("reply_to",8);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.reply_to.len   = str.length();
				abp_properties.reply_to.bytes = str.data();
			}
			opt = options.at("expiration",10);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_EXPIRATION_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.expiration.len   = str.length();
				abp_properties.expiration.bytes = str.data();
			}
			opt = options.at("timestamp",9);
			abp_properties._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
			if(opt.is_long()) {
				abp_properties.timestamp = static_cast<int64_t>(opt);
			}else{
				abp_properties.timestamp = time::now()/1000;
			}
			opt = options.at("message_id",10);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_MESSAGE_ID_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.message_id.len   = str.length();
				abp_properties.message_id.bytes = str.data();
			}
			opt = options.at("type",4);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_TYPE_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.type.len   = str.length();
				abp_properties.type.bytes = str.data();
			}
			opt = options.at("user_id",7);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_USER_ID_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.user_id.len   = str.length();
				abp_properties.user_id.bytes = str.data();
			}
			opt = options.at("app_id",6);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_APP_ID_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.app_id.len   = str.length();
				abp_properties.app_id.bytes = str.data();
			}
			opt = options.at("cluster_id",10);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CLUSTER_ID_FLAG;
				php::string& str = static_cast<php::string&>(opt);
				abp_properties.cluster_id.len   = str.length();
				abp_properties.cluster_id.bytes = str.data();
			}
			ctx->rv = amqp_basic_publish(ctx->self->conn_, 1, 
				ctx->self->producer_->opt_exchange, abp_rkey,
				ctx->self->producer_->opt_mandatory,
				ctx->self->producer_->opt_immediate, &abp_properties, abp_body);
		}else{
			abp_properties.timestamp = time::now()/1000;
			abp_properties._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
			ctx->rv = amqp_basic_publish(ctx->self->conn_, 1, 
				ctx->self->producer_->opt_exchange, abp_rkey,
				ctx->self->producer_->opt_mandatory,
				ctx->self->producer_->opt_immediate, &abp_properties, abp_body);
		}
		ctx->self->reset_timer();
	}
	void client_implement::flush_wk(uv_work_t* req) {
		// 通过 close_channel 模拟实现 flush 功能
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->conn_ == nullptr) {
			ctx->rv = -2;
			return;
		}
		amqp_maybe_release_buffers_on_channel(ctx->self->conn_, 1);
		amqp_channel_close(ctx->self->conn_, 1, AMQP_REPLY_SUCCESS);
		amqp_channel_open(ctx->self->conn_, 1);
		amqp_rpc_reply_t reply = amqp_get_rpc_reply(ctx->self->conn_);
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = reply.library_error;
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = -1;
		}else{
			ctx->rv = 0;
			ctx->self->reset_timer();
		}
	}
	void client_implement::consume_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->conn_ == nullptr) {
			ctx->rv = -2;
			return;
		}
		amqp_rpc_reply_t  reply;
		amqp_maybe_release_buffers(ctx->self->conn_);
		message* msg = static_cast<php::object&>(ctx->msg).native<message>();
		
		int64_t timeout = static_cast<int64_t>(ctx->key);
RECEIVE_NEXT:
		if(timeout > 0) {
			struct timeval to { timeout / 1000, (timeout % 1000) * 1000};
			reply = amqp_consume_message(ctx->self->conn_, &msg->envelope_, &to, 0);
		}else{
			reply = amqp_consume_message(ctx->self->conn_, &msg->envelope_, nullptr, 0);
		}
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			if(reply.library_error != AMQP_STATUS_TIMEOUT) {
				ctx->self->destroy();
			}
			ctx->rv = reply.library_error;
			// 不是很理解 driver 示例 consumer 中对异常情况读取 frame 的处理流程是为了解决什么问题
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = -1;
		}else{
			ctx->rv = 0;
			ctx->self->reset_timer();
		}
	}
	void client_implement::confirm_message_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		message* msg = static_cast<php::object&>(ctx->msg).native<message>();
		ctx->rv = amqp_basic_ack(ctx->self->conn_, 1, msg->envelope_.delivery_tag, 0);
		ctx->self->reset_timer();
	}
	void client_implement::reject_message_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		message* msg = static_cast<php::object&>(ctx->msg).native<message>();
		ctx->rv = amqp_basic_reject(ctx->self->conn_, 1, msg->envelope_.delivery_tag, ctx->key.is_true());
		ctx->self->reset_timer();
	}
	void client_implement::destroy_message_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		message* msg = static_cast<php::object&>(ctx->msg).native<message>();
		amqp_destroy_envelope(&msg->envelope_);
	}
	void client_implement::destroy_message_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		// destroy message 是 consume 的延续流程
		ctx->co->next(std::move(ctx->msg));
		delete ctx;
	}
	void client_implement::destroy_wk(uv_work_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		self->destroy();
	}
	void client_implement::destroy_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<client_implement*>(req->data);
	}
	void client_implement::timer_wk(uv_timer_t* tm) {
		client_implement* self = (client_implement*)tm->data;
		amqp_frame_t beat;
    	beat.channel = 0;
		beat.frame_type = AMQP_FRAME_HEARTBEAT;
		if (amqp_send_frame(self->conn_, &beat) == AMQP_STATUS_OK) {
			if(self->is_producer) {
				struct timeval to {0, 0};
				amqp_simple_wait_frame_noblock(self->conn_, &beat, &to);
			}
			self->reset_timer();
		}else{
			self->destroy();
		}
	}
	void client_implement::error_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		int error = ctx->rv;
		if(error == -2) {
			ctx->co->fail("rabbitmq client already closed");
		}else if(error == -1) {
			ctx->co->fail("rabbitmq server failed");
		}else if(error == 0) {
			ctx->co->next();
		}else{
			ctx->co->fail(amqp_error_string2(error), error);
		}
		delete ctx;
	}
}
}
}
