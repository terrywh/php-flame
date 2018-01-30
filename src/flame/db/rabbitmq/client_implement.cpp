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
	std::shared_ptr<php_url> client_implement::parse_url(const php::string& url) {
		std::shared_ptr<php_url> url_ = php::parse_url(url.c_str(), url.length());
		if(strncasecmp(url_->scheme, "amqp", 4) != 0) {
			throw php::exception(
				amqp_error_string2(AMQP_STATUS_BAD_URL),
				AMQP_STATUS_BAD_URL);
		}
		return url_;
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
			AMQP_DEFAULT_MAX_CHANNELS, AMQP_DEFAULT_FRAME_SIZE, 60,
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
	}
	void client_implement::destroy(bool close_channel) {
		if(conn_ == nullptr) return;
		if(close_channel) amqp_channel_close(conn_, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn_);
		conn_ = nullptr;
	}
	void client_implement::subscribe(const php::string& q) {
		amqp_bytes_t queue {q.length(), (void*)q.c_str()};
		
		amqp_basic_consume(conn_, 1, queue, amqp_empty_bytes,
			consumer_->opt_no_local,
			consumer_->opt_no_ack,
			consumer_->opt_exclusive,
			consumer_->opt_arguments);
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
		
		abp_body.len   = ctx->msg.length();
		abp_body.bytes = ctx->msg.data();
		
		if(ctx->key.is_string()) {
			abp_rkey.len   = ctx->key.length();
			abp_rkey.bytes = ctx->key.data();
		}else{
			abp_rkey = amqp_empty_bytes;
		}
		amqp_basic_properties_t abp_properties;
		std::memset(&abp_properties, 0, sizeof(amqp_basic_properties_t));
		php::array& options = ctx->rv;
		if(options.is_array()) {
			php::value opt = options.at("content_type",12);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
				php::string& str = opt;
				abp_properties.content_type.len   = str.length();
				abp_properties.content_type.bytes = str.data();
			}
			opt = options.at("content_encoding",12);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
				php::string& str = opt;
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
				php::string& str = opt;
				abp_properties.correlation_id.len   = str.length();
				abp_properties.correlation_id.bytes = str.data();
			}
			opt = options.at("reply_to",8);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
				php::string& str = opt;
				abp_properties.reply_to.len   = str.length();
				abp_properties.reply_to.bytes = str.data();
			}
			opt = options.at("expiration",10);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_EXPIRATION_FLAG;
				php::string& str = opt;
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
				php::string& str = opt;
				abp_properties.message_id.len   = str.length();
				abp_properties.message_id.bytes = str.data();
			}
			opt = options.at("type",4);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_TYPE_FLAG;
				php::string& str = opt;
				abp_properties.type.len   = str.length();
				abp_properties.type.bytes = str.data();
			}
			opt = options.at("user_id",7);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_USER_ID_FLAG;
				php::string& str = opt;
				abp_properties.user_id.len   = str.length();
				abp_properties.user_id.bytes = str.data();
			}
			opt = options.at("app_id",6);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_APP_ID_FLAG;
				php::string& str = opt;
				abp_properties.app_id.len   = str.length();
				abp_properties.app_id.bytes = str.data();
			}
			opt = options.at("cluster_id",10);
			if(opt.is_string()) {
				abp_properties._flags |= AMQP_BASIC_CLUSTER_ID_FLAG;
				php::string& str = opt;
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
	}
	void client_implement::flush_wk(uv_work_t* req) {
		// 通过 close_channel 模拟实现 flush 功能
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->conn_ == nullptr) {
			ctx->rv = -2;
			return;
		}
		amqp_channel_close(ctx->self->conn_, 1, AMQP_REPLY_SUCCESS);
		amqp_channel_open(ctx->self->conn_, 1);
		amqp_rpc_reply_t reply = amqp_get_rpc_reply(ctx->self->conn_);
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = reply.library_error;
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = -1;
		}
	}
	void client_implement::consume_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->conn_ == nullptr) {
			ctx->rv = -2;
			return;
		}
		amqp_rpc_reply_t  reply;
		amqp_envelope_t*  envelope = new amqp_envelope_t;
		amqp_maybe_release_buffers(ctx->self->conn_);

		int64_t timeout = ctx->rv;
		if(timeout > 0) {
			struct timeval to { timeout / 1000, (timeout % 1000) * 1000};
			reply = amqp_consume_message(ctx->self->conn_, envelope, &to, 0);
		}else{
			reply = amqp_consume_message(ctx->self->conn_, envelope, nullptr, 0);
		}
		if(reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			// 不是很理解 driver 示例代码中对异常情况读取 frame 的处理流程是为了解决什么问题
			// if(reply.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
			// 	amqp_frame_t frame;
			// 	if(amqp_simple_wait_frame(ctx->self->conn_, &frame) == AMQP_STATUS_OK && AMQP_FRAME_METHOD == frame.frame_type) {
			// 		std::printf("frame.method.id: 0x%08x\n", frame.payload.method.id);

			// 		// switch(frame.payload.method.id) {
			// 		// 	case AMQP_BASIC_RETURN_METHOD:
			// 		// 	break;
			// 		// 	case AMQP_BASIC_ACK_METHOD:
			// 		// 	break;
			// 		// 	case AMQP_CHANNEL_CLOSE_METHOD:
			// 		// 	case AMQP_CONNECTION_CLOSE_METHOD:
			// 		// 	break;
			// 		// }
			// 	}
			// }
			ctx->self->destroy();
			ctx->rv = reply.library_error;
			return;
		}else if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
			ctx->self->destroy();
			ctx->rv = -1;
			return;
		}
		
		ctx->rv.ptr(envelope);
	}
	void client_implement::confirm_envelope_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		php::object& obj = ctx->msg;
		message* msg = obj.native<message>();
		ctx->rv = amqp_basic_ack(ctx->self->conn_, 1, msg->envelope_->delivery_tag, 0);
	}
	void client_implement::reject_envelope_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		php::object& obj = ctx->msg;
		message* msg = obj.native<message>();
		ctx->rv = amqp_basic_reject(ctx->self->conn_, 1, msg->envelope_->delivery_tag, ctx->key.is_true());
	}
	void client_implement::destroy_envelope_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		amqp_destroy_envelope(ctx->msg.ptr<amqp_envelope_t>());
	}
	void client_implement::destroy_wk(uv_work_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		self->destroy();
	}
	void client_implement::destroy_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<client_implement*>(req->data);
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
