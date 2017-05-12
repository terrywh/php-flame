#pragma once

namespace net { namespace http {
	class write_buffer {
	public:
		// (1)
		write_buffer(std::string&& str)
		: data_(std::move(str)) {
			used_ = data_.c_str();
			size_ = data_.length();
		}
		write_buffer(const char* data, std::size_t size)
		: data_() // TODO 提供自定义的 Allocator 减少 data_ 代价
		, used_(data)
		, size_(size) {
		}

		// write_buffer(write_buffer&& buf)
		// : data_(std::move(buf.data_)) {
		// 	used_ = data_.data();
		// 	size_ = buf.size_;
		// }
		// !!! 复制流程有可能会对应到 (1) 的对象，但就目前用法来说，没有问题：
		// boost::asio::async_write 流程中可能会若干次的 copy 上层的容器，间接
		// 复制 write_buffer 元素，这里为了减少复制开销，不复制实际 data_ 字符串
		write_buffer(const write_buffer& buf)
		: data_() {
			used_ = buf.used_;
			size_ = buf.size_;
		}

		operator boost::asio::const_buffer() const {
			boost::asio::const_buffer buffer(used_, size_);
			return buffer;
		}
		const char* data() {
			return used_;
		}
		std::size_t size() {
			return size_;
		}
	private:
		std::string data_;
		const char* used_;
		std::size_t size_;
	};
}}
