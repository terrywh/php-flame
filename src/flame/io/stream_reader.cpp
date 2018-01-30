#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "stream_reader.h"

namespace flame {
namespace io {
	stream_reader::stream_reader(uv_stream_t* s)
	: cli_(s)
	, tm_((uv_timer_t*)malloc(sizeof(uv_timer_t))) {
		uv_timer_init(flame::loop, tm_);
		tm_ ->data = this;
		cli_->data  = this;
	}
	stream_reader::~stream_reader() {
		close();
	}
	void stream_reader::close(int err) {
		if(co_) {
			coroutine* co = co_;
			co_ = nullptr;
			// 由于 co->next() 本身可能引起下一次的读取动作，
			// 故 self->co_ = nullptr 必须置于 co->next() 之前
			// 否则可能导致将下次的 co_ 被清理
			if(err == 0) co->next();
			else co->fail(uv_strerror(err), err);
		}
		if(tm_) {
			uv_timer_stop(tm_);
			uv_close((uv_handle_t*)tm_, flame::free_handle_cb);
			tm_ = nullptr;
		}
		if(cli_) {
			uv_read_stop(cli_);
			cli_ = nullptr;
		}
	}
	void stream_reader::read() {
		if(!cli_) throw php::exception("failed to read: socket is already closed");
		d_type = 0;
		d_size = 0;
		d_endl = nullptr;
		
		co_ = coroutine::current;
		if(read_from_buf()) {
			// 仿造异步方式，在下个循环返回
			uv_timer_start(tm_, return_cb, 0, 0);
		}else{
			uv_read_start((uv_stream_t*)cli_, alloc_cb, read_cb);
		}
	}
	void stream_reader::read(size_t size) {
		if(!cli_) throw php::exception("failed to read: socket is already closed");
		co_    = coroutine::current;
		d_type = 1;
		d_size = size;
		d_endl = nullptr;
		
		co_ = coroutine::current;
		if(read_from_buf()) {
			uv_timer_start(tm_, return_cb, 0, 0);
		}else{
			uv_read_start((uv_stream_t*)cli_, alloc_cb, read_cb);
		}
	}
	void stream_reader::read(const php::string& endl) {
		if(!cli_) throw php::exception("failed to read: socket is already closed");
		co_    = coroutine::current;
		d_type = 2;
		d_size = 0;
		d_endl = endl;
		
		co_ = coroutine::current;
		if(read_from_buf()) {
			uv_timer_start(tm_, return_cb, 0, 0);
		}else{
			uv_read_start((uv_stream_t*)cli_, alloc_cb, read_cb);
		}
	}
	void stream_reader::read_all() {
		d_type = 3;
		d_size = 0;
		d_endl = nullptr;
		
		// 使用 cli_ 作为读取结束的标记
		co_ = coroutine::current;
		if(cli_) {
			uv_read_start((uv_stream_t*)cli_, alloc_cb, read_cb);
		}else{
			uv_timer_start(tm_, return_cb, 0, 0);
		}
	}
	void stream_reader::return_cb(uv_timer_t* handle) {
		stream_reader* self = reinterpret_cast<stream_reader*>(handle->data);
		// 由于 co->next() 本身可能引起下一次的读取动作，
		// 故 self->co_ = nullptr 必须置于 co->next() 之前
		// 否则可能导致将下次的 co_ 被清理
		coroutine* co = self->co_;
		self->co_ = nullptr;
		co->next(std::move(self->rv_));
	}
	void stream_reader::alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf) {
		stream_reader* self = reinterpret_cast<stream_reader*>(handle->data);
		buf->base = self->buf_.rev(2048);
		buf->len  = 2048;
	}
	void stream_reader::read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
		stream_reader* self = static_cast<stream_reader*>(handle->data);
		if(nread == UV_EOF) {
			// 特殊读取方式：读到结束
			if(self->d_type == 3) {
				self->rv_ = std::move(self->buf_);
				return_cb(self->tm_); // 返回现有的数据 或 NULL
			}
			self->close();
		}else if(nread < 0) {
			self->close(nread);
		}else if(nread == 0) {
			// again
		}else{
			self->buf_.adv(nread);
			if(self->read_from_buf()) {
				uv_read_stop((uv_stream_t*)self->cli_);
				return_cb(self->tm_);
			}
		}
	}
	bool stream_reader::read_from_buf() {
		switch(d_type) {
		case 0: // 读取一定量数据
			if(buf_.size() > 0) {
				rv_ = std::move(buf_);
				return true;
			}
		break;
		case 1: // 读取一定大小
			if(buf_.size() >= d_size) {
				rv_ = std::move(buf_);
				php::string& str = rv_;
				if(str.length() > d_size) {
					std::memcpy(buf_.put(str.length() - d_size), str.c_str() + d_size, str.length() - d_size);
					// 1. 直接使用当前的内存空间，可能导致 json_decode 等操作失败（本字符串没有以 \0 结束）
					// str.length() = d_size;
					// 2. resize 调整长度后在长度位置后添加 '\0' 结束
					str.resize(d_size);
				}
				return true;
			}
		break;
		case 2: // 结束符方式
		{
			auto ff = std::search(buf_.data(), buf_.data() + buf_.size(),
				d_endl.data(), d_endl.data() + d_endl.length());
			if(ff != buf_.data() + buf_.size()) { // 找到
				ff += d_endl.length();
				d_size = ff - buf_.data();

				rv_ = std::move(buf_);
				php::string& str = rv_;
				if(str.length() > d_size) {
					std::memcpy(buf_.put(str.length() - d_size), str.c_str() + d_size, str.length() - d_size);
					// 1. 直接使用当前的内存空间，可能导致 json_decode 等操作失败（本字符串没有以 \0 结束）
					// str.length() = d_size;
					// 2. resize 调整长度后在长度位置后添加 '\0' 结束
					str.resize(d_size);
				}
				return true;
			}
		}
		break;
		case 3: // 读取到末尾 一定是 false
			return false;
		default:
			throw php::exception("failed to read: illegal read method");
		}
	}
	
}
}
