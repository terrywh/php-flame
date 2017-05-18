#include "../../vendor.h"
#include "../../core.h"
#include "server_response.h"
#include "header.h"

namespace net { namespace http {
	void server_response::init(php::extension_entry& extension) {
		php::class_entry<server_response> class_server_response("flame\\net\\http\\server_response");
		class_server_response.add<&server_response::__destruct>("__destruct");
		class_server_response.add<&server_response::write_header>("write_header", {
			php::of_integer("code"),
			php::of_string("reason"),
		});
		class_server_response.add<&server_response::write>("write", {
			php::of_string("data"),
		});
		class_server_response.add<&server_response::end>("end", {
			php::of_string("data"),
		});
		class_server_response.add(php::property_entry("header", nullptr));
		extension.add(std::move(class_server_response));
	}
	void server_response::init(evhttp_request* evreq) {
		req_ = evreq;
		evhttp_request_set_on_complete_cb(req_, server_response::complete_handler, this);
		php::object hdr = php::object::create<header>();
		header_ = hdr.native<header>();
		header_->init(evhttp_request_get_output_headers(req_));
		prop("header") = hdr;
	}

	server_response::server_response()
	: header_sent_(false)
	, completed_(false)
	, chunk_(evbuffer_new()) {}

	server_response::~server_response() {
		evbuffer_free(chunk_);
	}

	php::value server_response::__destruct(php::parameters& params) {
		if(!completed_) {
			completed_ = true;
			// 由于当前对象将销毁，不能继续捕捉请求完成的回调
			evhttp_request_set_on_complete_cb(req_, nullptr, nullptr);
			if(!header_sent_) {
				evhttp_send_reply_start(req_, 204, nullptr);
			}
			evhttp_send_reply_end(req_);
		}
		return nullptr;
	}

	php::value server_response::write_header(php::parameters& params) {
		if(header_sent_) throw php::exception("write_header failed: header already sent");
		if(params.length() > 1) {
			zend_string* reason = params[1];
			evhttp_send_reply_start(req_, params[0], reason->val);
		}else{
			evhttp_send_reply_start(req_, params[0], nullptr);
		}
		header_sent_ = true;
		// libevent 没有提供 evhttp_send_reply_start 完成回调的功能，
		// 参见：v2.1.8 http.c:2838
		return nullptr;
	}

	php::value server_response::write(php::parameters& params) {
		if(completed_) throw php::exception("write failed: response aready ended");
		if(!header_sent_) {
			evhttp_send_reply_start(req_, 200, nullptr);
		}
		wbuffer_.push_back(params[0]);
		php::string& data = wbuffer_.back();
		evbuffer_add_reference(chunk_, data.data(), data.length(), nullptr, nullptr);
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			evhttp_send_reply_chunk_with_cb(req_, chunk_,
				reinterpret_cast<void (*)(struct evhttp_connection*, void*)>(server_response::complete_handler), this);
			return nullptr;
		});
	}

	void server_response::complete_handler(struct evhttp_request* _, void* ctx) {
		// 注意：上面 evhttp_request* 指针不能使用（有强制类型转换的释放方式）
		server_response* self = reinterpret_cast<server_response*>(ctx);
		// 发送完毕后需要清理缓存（参数引用）
		self->wbuffer_.clear();
		if(self->cb_.is_empty()) return;
		// callback 调用机制请参考 tcp_socket 内相关说明
		php::callable cb = std::move(self->cb_);
		cb();
	}

	php::value server_response::end(php::parameters& params) {
		if(completed_) throw php::exception("end failed: response already ended");
		if(!header_sent_) {
			evhttp_send_reply_start(req_, 200, nullptr);
		}
		if(params.length() > 0) {
			wbuffer_.push_back(params[0]);
			php::string& data = wbuffer_.back();
			evbuffer_add_reference(chunk_, data.data(), data.length(), nullptr, nullptr);
			evhttp_send_reply_chunk(req_, chunk_);
		}
		completed_ = true;
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			evhttp_send_reply_end(req_);
			return nullptr;
		});
	}


}}
