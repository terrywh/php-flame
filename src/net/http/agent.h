#pragma once

namespace net { namespace http {
	class client_request;
	struct agent_socket_wrapper {
		std::chrono::time_point<std::chrono::steady_clock> expire_;
		std::shared_ptr<tcp::socket>                       socket_;
	};
	class agent: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		agent();
		~agent();
		php::value __construct(php::parameters& params);
		// 需要被 request 反复调用，获取可用的连接
		void acquire(const std::string& domain, const std::string& port, std::function<void (const boost::system::error_code&, std::shared_ptr<tcp::socket>)> handler);
		void release(const std::string& domain, const std::string& port, std::shared_ptr<tcp::socket> sock);
		// 在 module_startup 中初始化

		void start_sweep();
	private:
		static agent* default_agent_;
		tcp::resolver                                                resolver_;
		std::map<std::string, std::forward_list<agent_socket_wrapper>> socket_;
		boost::asio::steady_timer timer_;
		std::chrono::seconds      ttl_;

		friend php::value request(php::parameters& params);
	};
}}
