#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../time/time.h"
#include "log.h"
#include "logger.h"

namespace flame {
namespace log {
	logger::logger()
	: pipe_(nullptr)
	, path_(0)
	, file_(0) {
		sig_ = (uv_signal_t*)malloc(sizeof(uv_signal_t));
		uv_signal_init(flame::loop, sig_);
		sig_->data = this;
		uv_signal_start(sig_, signal_cb, SIGUSR2);
		uv_unref((uv_handle_t*)sig_);
	}
	logger::~logger() {
		close();
		uv_signal_stop(sig_);
		uv_close((uv_handle_t*)sig_, free_handle_cb);
	}
	void logger::close() {
		// 下面对象允许重复打开关闭
		if(pipe_) {
			uv_close((uv_handle_t*)pipe_, free_handle_cb);
			pipe_ = nullptr;
		}
		if(file_ > 0) { // 排除 file_ = 1 / file_ = 2 标准输出的情况
			uv_fs_t req;
			uv_fs_close(flame::loop, &req, file_, nullptr);
			file_ = 0;
		}
	}
	php::value logger::set_output(php::parameters& params) {
		if(params.length() > 0) {
			rotate(params[0]);
		}else{
			throw php::exception("failed to set output: stderr / stdout / stdlog or filepath is required");
		}
		return nullptr;
	}
	void logger::init(bool def) {
		if(def) {
			php::register_shutdown_function(php::callable([] (php::parameters& params) -> php::value {
				default_logger->panic();
				return nullptr;
			}));
		}
		rotate();
	}
	void logger::rotate() {
		close();
		
		if(path_.length() == 0 || path_.length() == 6 && (std::strncmp(path_.c_str(), "stderr", 6) == 0
				|| std::strncmp(path_.c_str(), "stdlog", 6) == 0)) {
			file_ = -2;
		}else if(path_.length() == 6 && std::strncmp(path_.c_str(), "stdout", 6) == 0) {
			file_ = -1;
		}else{
			uv_fs_t req;
			file_ = uv_fs_open(flame::loop, &req, path_.c_str(), O_APPEND | O_CREAT | O_WRONLY, 0777, nullptr);
			if(file_ < 0) {
				throw php::exception("cannot open logger output file");
			}
			pipe_ = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
			uv_pipe_init(flame::loop, pipe_, 0);
			pipe_->data = this;
			uv_pipe_open(pipe_, file_);
			uv_unref((uv_handle_t*)pipe_);
		}
	}
	void logger::rotate(const php::string& path) {
		path_ = path;
		rotate();
	}
	typedef struct write_request_t {
		coroutine*   co;
		logger*    self;
		php::string str;
		uv_write_t  req;
	} write_request_t;
	void logger::write_cb(uv_write_t* req, int status) {
		write_request_t* ctx = reinterpret_cast<write_request_t*>(req->data);
		if(ctx->co != nullptr) ctx->co->next();
		delete ctx;
	}
	void logger::panic() {
		// std::fprintf(stderr, "[%s] %.*s", time::datetime(time::now()), errlen, errstr);
		if(file_ > 0) {
			size_t errlen = 0;
			std::string errstr;
			if(php::exception::has()) {
				php::exception ex = php::exception::get();
				errstr = fmt::format("[{0}] (EXCEPTION) {1}:{2} {3}", 
					time::datetime(time::now()),
					ex.file().c_str(),
					ex.line(),
					ex.message().c_str());
			}else if(PG(last_error_type)) {
				errstr = fmt::format("[{0}] ({1}) {2}:{3} {4}", 
					time::datetime(time::now()),
					php::error_type_name(PG(last_error_type)),
					PG(last_error_file),
					PG(last_error_lineno),
					PG(last_error_message));
			}else {
				return;
			}
			::lseek(file_, SEEK_END, 0);
			::write(file_, errstr.c_str(), errstr.length());
			uv_fs_t req;
			uv_fs_close(flame::loop, &req, file_, nullptr);
			file_ = 0;
		}
		// ::exit(-1);
	}
	void logger::signal_cb(uv_signal_t* handle, int signal) {
		logger* self = reinterpret_cast<logger*>(handle->data);
		self->rotate();
	}
	php::value logger::fail(php::parameters& params) {
		if(write(make_buffer(" (FAIL)", params))) {
			return flame::async(this);
		}
		return nullptr;
	}
	php::value logger::warn(php::parameters& params) {
		if(write(make_buffer(" (WARN)", params))) {
			return flame::async(this);
		}
		return nullptr;
	}
	php::value logger::info(php::parameters& params) {
		if(write(make_buffer(" (INFO)", params))) {
			return flame::async(this);
		}
		return nullptr;
	}
	php::value logger::write(php::parameters& params) {
		if(write(make_buffer("", params))) {
			return flame::async(this);
		}
		return nullptr;
	}
	php::string logger::make_buffer(const std::string& level, php::parameters& params) {
		php::buffer out;
		out.add('[');
		std::memcpy(out.put(19), time::datetime(time::now()), 19);
		out.add(']');
		if(level.length() > 0) {
			std::memcpy(out.put(level.length()), level.c_str(), level.length());
		}
		for(int i=0;i<params.length();++i) {
			out.add(' ');
			if(params[i].is_array()) { // 自动输出 JSON 数据
				php::string str = php::json_encode(params[i]);
				std::memcpy(out.put(str.length()), str.c_str(), str.length());
			}else{
				php::string& str = params[i].to_string();
				std::memcpy(out.put(str.length()), str.c_str(), str.length());
			}
		}
		out.add('\n');
		return std::move(out);
	}
	bool logger::write(const php::string& out) {
		if(file_ == -2) {
			std::fputs(out.c_str(), stderr);
			return false;
		}else if(file_ == -1) {
			std::fputs(out.c_str(), stdout);
			return false;
		}else{
			write_request_t* ctx = new write_request_t {
				coroutine::current, this, std::move(out)
			};
			ctx->req.data = ctx;
			uv_buf_t data {.base = ctx->str.data(), .len = ctx->str.length()};
			uv_write(&ctx->req, (uv_stream_t*)pipe_, &data, 1, write_cb);
			return true;
		}
	}
}
}
