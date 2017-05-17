#include "../../vendor.h"
#include "../../core.h"
#include "response.h"
#include "init.h"

namespace net { namespace http {
	void server_response::init(php::extension_entry& extension) {
		php::class_entry<response> class_server_response("flame\\net\\http\\server_response");
		class_server_response.add<&server_response::__destruct>("__destruct");
		class_server_response.add<&server_response::end>("end", {
			php::of_string("data"),
		});
		extension.add(std::move(class_server_response));
	}
	server_response::server_response()
	: wbuffer_(evbuffer_new())
	, completed_(false) {}

	server_response::~server_response() {
		evbuffer_free(wbuffer_);
	}

	php::value server_response::__destruct(php::parameters& params) {
		if(!completed_) {
			completed_ = true;
			// 由于当前对象将销毁，不能继续捕捉请求完成的回调
			evhttp_request_set_on_complete_cb(req_, nullptr, nullptr);
			evhttp_send_reply(req_, 204, nullptr, nullptr);
		}
		return nullptr;
	}

	void server_response::complete_handler(struct evhttp_request* req_, void* ctx) {
		response* self = reinterpret_cast<response*>(ctx);
		// 发送完毕后需要清理缓存（参数引用）
		self->wbuffer_cache_.clear();
		if(self->cb_.is_empty()) return;
		// callback 调用机制请参考 tcp_socket 内相关说明
		php::callable cb = std::move(self->cb_);
		cb();
	}

	php::value server_response::end(php::parameters& params) {
		if(completed_) throw php::exception("end failed: response already ended");
		if(params.length() > 0) {
			wbuffer_cache_.push_back(params[0]);
			php::string& data = wbuffer_cache_.back();
			evbuffer_add_reference(wbuffer_, data.data(), data.length(), nullptr, nullptr);
		}
		completed_ = true;
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			evhttp_send_reply(req_, 200, nullptr, wbuffer_);
			return nullptr;
		});
	}


}}
