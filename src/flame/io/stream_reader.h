#pragma once

namespace flame {
namespace io {
	class stream_reader {
	public:
		stream_reader(uv_stream_t* uv);
		~stream_reader();
		void close(int error = 0);
		void read();
		void read(size_t size);
		void read(const php::string& delim);
		void read_all();
		
	private:
		uv_stream_t*   cli_;
		coroutine*      co_; // 当前协程
		php::buffer    buf_;
		
		size_t           d_size;
		php::string      d_endl;
		int              d_type;
		
		php::string     rv_;
		uv_timer_t*     tm_;
		
		bool read_from_buf();
		
		static void return_cb(uv_timer_t* handle);
		static void  alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf);
		static void   read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
	};
}
}
