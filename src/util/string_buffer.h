#pragma once

namespace util {
	class string_buffer {
	public:
		php::string slice(size_t length);
		php::string slice_first();
		inline bool empty() {
			return length_ == 0;
		}
		size_t length() {
			return length_;
		}
		inline void append(const php::string& data) {
			queue_.push_back(data);
			length_ += data.length();
		}
		inline php::string& append(size_t size) {
			queue_.emplace_back(size, false);
			return queue_.back();
		}
		inline php::string& back() {
			return queue_.back();
		}
		ssize_t find(const char* delim, size_t size);
	private:
		std::deque<php::string> queue_;
		size_t                  length_;
		// 第一个块内偏移
		size_t                  offset_;
	};
}
