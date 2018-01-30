#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../time/time.h"
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
			if(!file_) throw php::exception("failed to open output file");
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
		ctx->co->next();
		delete ctx;
	}
	bool logger::write(const std::string& data) {
		php::buffer out;
		out.add('[');
		std::memcpy(out.put(19), time::datetime(time::now()), 19);
		out.add(']');
		out.add(' ');
		std::memcpy(out.put(data.length()), data.c_str(), data.length());
		out.add('\n');
		return write(std::move(out));
	}
	void logger::panic() {
		php::object ex1(EG(exception), true);
		EG(exception) = nullptr;
		
		if(file_ >= 0) {
			// 若设置了文件输出，额外向文件进行一次输出
			panic_to_file(ex1);
		}
		// 保持默认的输出
		zend_exception_error(ex1, E_ERROR);
		::exit(-1);
	}
	void logger::panic_to_file(php::object& ex) {
		// 以下相关代码参考 zend_exception.c 中 zend_exception_error 函数相关代码
		char   errstr[4096];
		size_t errlen = sizeof(errstr);
		if(static_cast<zend_class_entry*>(ex) == zend_ce_parse_error) {
			php::string data = ex.prop("message", 7);
			php::string file = ex.prop("file", 4);
			zend_long   line = ex.prop("line", 4);
			
			errlen = sprintf(errstr, "[%s] (PANIC) %.*s in %.*s:%d\n",
				time::datetime(time::now()), data.length(), data.c_str(),
				file.length(), file.c_str(), line);
			goto ERROR_OUTPUT;
		}else if(ex.is_instance_of(zend_ce_throwable)) {
			php::string data;
			// 调用过程本身还会产生一个异常（一定的）
			zend_call_method_with_0_params((zval*)&ex, ex, nullptr, "__tostring", (zval*)&data);
			if(data.is_string()) {
				php::string file = ex.prop("file", 4);
				zend_long   line = ex.prop("line", 4);
				
				errlen = sprintf(errstr, "[%s] (PANIC) %.*s\n",
					time::datetime(time::now()), data.length(), data.c_str(),
					file.length(), file.c_str(), line);
				goto ERROR_OUTPUT;
			}
		}
		errlen = sprintf(errstr, "[%s] (PANIC) Uncaught exception '%s'\n",
			time::datetime(time::now()),
			static_cast<zend_class_entry*>(ex)->name->val);
ERROR_OUTPUT:
		::lseek(file_, SEEK_END, 0);
		::write(file_, errstr, errlen);
		
		uv_fs_t req;
		uv_fs_close(flame::loop, &req, file_, nullptr);
		file_ = 0;
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
			std::fprintf(stderr, out.c_str());
			return false;
		}else if(file_ == -1) {
			std::fprintf(stdout, out.c_str());
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
