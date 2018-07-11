#pragma once

namespace flame {
	class dynamic_buffer {
	public:
		typedef boost::asio::const_buffer const_buffers_type;
		typedef boost::asio::mutable_buffer mutable_buffers_type;
		dynamic_buffer(php::buffer& buffer)
		: buffer_(buffer) {
			
		}
		dynamic_buffer(dynamic_buffer&& db)
		: buffer_(db.buffer_) {
			
		}
		dynamic_buffer(const dynamic_buffer& db)
		: buffer_(db.buffer_) {

		}
		std::size_t max_size() {
			return buffer_.max_size();
		}
		std::size_t capacity() {
			return buffer_.capacity();
		}
		std::size_t size() {
			return buffer_.size();
		}
		// 可读取数据缓冲区
		const_buffers_type data() {
			return boost::asio::const_buffer(buffer_.data(), buffer_.size());
		}
		void consume(std::size_t n) {
			buffer_.consume(n);
		}
		mutable_buffers_type prepare(std::size_t n) {
			return boost::asio::mutable_buffer(buffer_.prepare(n), n);
		}
		void commit(std::size_t n) {
			buffer_.commit(n);
		}
	private:
		php::buffer& buffer_;
	};
}