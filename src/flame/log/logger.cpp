#include "../coroutine.h"
#include "../flame.h"
#include "../time/time.h"
#include "logger.h"



namespace flame {
namespace log {
	logger::logger()
	: pipe_(nullptr)
	, file_(0) {

	}
	logger::~logger() {
		close();
	}
	void logger::close() {
		if(pipe_) {
			uv_close((uv_handle_t*)pipe_, free_handle_cb);
			pipe_ = nullptr;
		}
		if(file_) {
			uv_fs_t req;
			uv_fs_close(flame::loop, &req, file_, nullptr);
			file_ = 0;
		}
	}
	php::value logger::rotate(php::parameters& params) {
		if(params.length() > 0) {
			rotate(params[0]);
		}else{
			rotate();
		}
		return nullptr;
	}
	void logger::rotate() {
		close();
		pipe_ = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
		uv_pipe_init(flame::loop, pipe_, 0);
		pipe_->data = this;
		if(path_.length() == 0 || path_.length() == 6 && std::strncmp(path_.c_str(), "stderr", 6) == 0) {
			uv_pipe_open(pipe_, 2);
			file_ = 0;
		}else if(path_.length() == 6 && std::strncmp(path_.c_str(), "stdout", 6) == 0 ||
			std::strncmp(path_.c_str(), "stdlog", 6) == 0) {
			uv_pipe_open(pipe_, 1);
			file_ = 0;
		}else{
			uv_fs_t req;
			file_ = uv_fs_open(flame::loop, &req, path_.c_str(), O_APPEND | O_CREAT | O_WRONLY, 0777, nullptr);
			if(!file_) throw php::exception("failed to open output file");
			uv_pipe_open(pipe_, file_);
		}
	}
	void logger::rotate(const php::string& path) {
		path_ = path;
		rotate();
	}
	typedef struct write_request_t {
		coroutine*   co;
		logger*    self;
		php::value   rv;
		php::string str;
		uv_write_t  req;
	} write_request_t;
	void logger::write_cb(uv_write_t* req, int status) {
		write_request_t* ctx = reinterpret_cast<write_request_t*>(req->data);
		ctx->co->next();
		delete ctx;
	}
	php::value logger::write(const std::string& level, php::parameters& params) {
		if(pipe_ == nullptr) return nullptr;
		php::buffer out;
		int64_t    st = time::now()/1000;
		struct tm* tt = std::localtime((time_t*)&st);
		sprintf(out.put(21), "[%04d-%02d-%02d %02d:%02d:%02d]",
			tt->tm_year + 1900,
			tt->tm_mon + 1,
			tt->tm_mday,
			tt->tm_hour,
			tt->tm_min,
			tt->tm_sec);
		std::memcpy(out.put(level.length()), level.c_str(), level.length());
		for(int i=0;i<params.length();++i) {
			php::string str = params[i].to_string();
			std::memcpy(out.put(str.length()), str.c_str(), str.length());
		}
		out.add('\n');
		write_request_t* ctx = new write_request_t {
			coroutine::current, this, this, std::move(out)
		};
		ctx->req.data = ctx;
		uv_buf_t data {.base = ctx->str.data(), .len = ctx->str.length()};
		uv_write(&ctx->req, (uv_stream_t*)pipe_, &data, 1, write_cb);
		return flame::async();
	}
	php::value logger::fail(php::parameters& params) {
		return write(" (FAIL) ", params);
	}
	php::value logger::warn(php::parameters& params) {
		return write(" (WARN) ", params);
	}
	php::value logger::info(php::parameters& params) {
		return write(" (INFO) ", params);
	}
	php::value logger::write(php::parameters& params) {
		return write(" ", params);
	}
	void logger::write(const php::string& str) {

	}
}
}
