#include "string_buffer.h"

namespace util {
	php::string string_buffer::slice(size_t size) {
		php::string data = queue_.front();
		queue_.pop_front();
		if(data.length() > size) {
			queue_.emplace_front(data.data() + size, data.length() - size);
			data.length() = size;
			length_ -= size;
			return data;
		}
		if(data.length() == size) {
			length_ -= size;
			return data;
		}
		php::buffer buffer;
		memcpy(buffer.put(data.length()), data.data(), data.length());
		length_ -= data.length();
		size    -= data.length();

		while(size > 0) {
			data = queue_.front();
			queue_.pop_front();
			if(data.length() > size) {
				memcpy(buffer.put(size), data.data(), size);
				queue_.emplace_front(data.data() + size, data.length() - size);
				size     = 0;
				length_ -= size;
			}else if(data.length() == size) {
				memcpy(buffer.put(size), data.data(), size);
				size     = 0;
				length_ -= size;
			}else{
				memcpy(buffer.put(size), data.data(), data.length());
				size    -= data.length();
				length_ -= data.length();
			}
		}
		return std::move(buffer);
	}
	php::string string_buffer::slice_first() {
		php::string data = queue_.front();
		queue_.pop_front();
		return std::move(data);
	}
	ssize_t string_buffer::find(const char* delim, size_t size) {
		size_t  i = 0;
		ssize_t l = 0;
		auto    q = queue_.begin();
		char*   c = q->data();
		while(l < length_ && q != queue_.end()) {
			if(*c == delim[i]) ++i;
			else i=0;
			++l;
			if(c - q->data() < q->length() - 1) ++c;
			else c = (++q)->data();
		}
		if(i == size) return l;
		return -1;
	}
}
