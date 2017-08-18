#include "stream_buffer.h"

namespace flame {
namespace net {
	char* stream_buffer::put(int n) {
		size_ += n;
		if(buffers_.empty()) {
			buffers_.emplace_back();
			return buffers_.back().put(n);
		}
		return buffers_.back().put(n);
	}
	char* stream_buffer::rev(int n) {
		// buffer 容器为空 或 最后一个容量不足 时新建 buffer 放入容器
		// if(buffers_.empty() || buffers_.back().capacity() - buffers_.back().size() < n) {
			buffers_.emplace_back(n);
		// }
		return buffers_.back().current();
	}
	void stream_buffer::adv(int n) {
		size_ += n;
		buffers_.back().adv(n);
	}
	bool stream_buffer::empty() {
		return size_ == 0;
	}
	unsigned int stream_buffer::size() {
		return size_;
	}
	php::string stream_buffer::get(unsigned int n) {
		// 这里不能使用引用，需要保持 data 一直有效
		php::buffer data = std::move(buffers_.front());
		// 1. 首个元素特殊处理
		// 实际逻辑与 2. 中基本一致，仅做数据调整，不做数据复制
		if(data.size() > n) {
			php::buffer left;
			std::memcpy(left.put(data.size() - n), data.data() + n, left.size());
			data.reset(n);
			n = 0;
			buffers_.pop_front();
			buffers_.push_front(std::move(left));
			// 剩余的部分位于头部
		}else if(data.size() == n) {
			n = 0;
			buffers_.pop_front();
		}else{
			n -= data.size();
			buffers_.pop_front();
		}
		// 2. 剩余处理逻辑
		while(n > 0 && !buffers_.empty()) {
			php::buffer& append = buffers_.front();
			if(append.size() > n) {
				php::buffer left;
				std::memcpy(left.put(append.size() - n), append.data() + n, left.size());
				// append.reset(n);
				std::memcpy(data.put(n), append.data(), n); // 追加
				n = 0;
				buffers_.pop_front(); // !!! pop 后上面 append 实际应用的对象会析构
				// 由于需要剩余的部分位于头部，pop 动作需要分别处理
				buffers_.push_front(std::move(left));
			}else if(append.size() == n) {
				std::memcpy(data.put(n), append.data(), n); // 追加
				n = 0;
				buffers_.pop_front(); // !!! pop 后上面 append 实际应用的对象会析构
			}else{
				std::memcpy(data.put(append.size()), append.data(), append.size()); // 追加
				n -= append.size();
				buffers_.pop_front(); // !!! pop 后上面 append 实际应用的对象会析构
			}
		}
		size_ -= data.size();
		return std::move(data);
	}
	php::string stream_buffer::get() {
		php::buffer data = std::move(buffers_.front());
		buffers_.pop_front();
		size_ -= data.size();
		return std::move(data);
	}
	int stream_buffer::find(const char* delim, unsigned int size, int offset) {
		auto p = buffers_.begin();
		int s = 0, i = 0, m = 0;
		while(offset > 0 && p != buffers_.end()) { // 跳过 offset
			if(offset > p->size()) {
				s      += p->size();
				offset -= p->size();
				++p;
			}else if(offset == p->size()) {
				s += offset;
				offset = 0;
				++p;
			}else{
				i  = p->size() - offset;
				s += i;
				offset = 0;
			}
		}
		while(p != buffers_.end()) { // 查找 delim
			if(p->data()[i] == delim[m]) {
				if(++m == size) {
					return s-1;
				}
			}else{
				m = 0;
			}
			++s;
			if(++i == p->size()) {
				++ p;
				i = 0;
			}
		}
		// 未找到
		return -1;
	}
}
}
