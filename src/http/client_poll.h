#pragma once
#include "../vendor.h"

namespace flame::http {
	class client_poll {
	public:
		client_poll();
		typedef void (*poll_callback_t)(const boost::system::error_code& error, client_poll* poll, curl_socket_t fd, int action);
		virtual ~client_poll();
		static client_poll* create_poll(boost::asio::io_context& io, curl_socket_t fd, poll_callback_t cb, void* data = nullptr);
		virtual void async_wait() = 0;
		void async_wait(int action);
		void* data;
	protected:
		curl_socket_t   fd_;
		int         action_;
		poll_callback_t cb_;
		void on_ready(const boost::system::error_code& error, int action);
	};

	class client_poll_tcp:public client_poll {
	public:
		// client_poll_tcp(boost::asio::io_context &io, boost::asio::ip::tcp proto, curl_socket_t fd);
		client_poll_tcp(boost::asio::io_context &io, boost::asio::ip::tcp proto, curl_socket_t fd);
		// client_poll_tcp(curl_socket_t fd);
		~client_poll_tcp();
		void async_wait() override;
	private:
		boost::asio::ip::tcp::socket sock_;
		// boost::asio::ip::tcp::socket& sock_;
		// boost::asio::posix::stream_descriptor sock_;
	};

	class client_poll_udp:public client_poll {
	public:
		client_poll_udp(boost::asio::io_context &io, boost::asio::ip::udp proto, curl_socket_t fd);
		~client_poll_udp();
		void async_wait() override;
	private:
		boost::asio::ip::udp::socket sock_;
	};
}
