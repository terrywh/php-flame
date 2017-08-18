#pragma once

namespace flame {
namespace net {
	class stream_buffer {
	private:
		std::deque<php::buffer> buffers_;
		unsigned int size_; // 缓存长度
	public:
		char* put(int n);
		char* rev(int n);
		void  adv(int n);
		bool         empty();
		unsigned int size();

		php::string get(unsigned int n);
		php::string get();

		int find(const char* delim, unsigned int size, int offset = 0);
	};
}
}
