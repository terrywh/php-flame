#pragma once

namespace flame {
namespace io {
	class stream_writer {
	public:
		stream_writer(uv_stream_t* s);
		void write(const php::string& data);
		void close();
	private:
		uv_stream_t*   cli_;
		static void write_cb(uv_write_t* handle, int status);
	};
}
}
