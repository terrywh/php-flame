#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace http {
	class client_connection;
	class client: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		client();
		php::value __construct(php::parameters& params);
		// 执行
		php::value exec(php::parameters& params);
		php::value get(php::parameters& params);
		php::value post(php::parameters& params);
		php::value put(php::parameters& params);
		php::value delete_(php::parameters& params);
		template <typename Stream, typename Handler>
		void acquire(std::shared_ptr<php::url> url, std::shared_ptr<Stream>& ptr, Handler && handler);
	private:
		php::value exec_ex(const php::object& req);
		// ----------------------------------------------------------------------------
		// 创建新连接
		template <typename Handler>
		void create(std::shared_ptr<php::url> url, std::shared_ptr<tcp::socket>& ptr, Handler && handler);
		template <typename Handler>
		void create(std::shared_ptr<php::url> url, std::shared_ptr<ssl::stream<tcp::socket>>& ptr, Handler && handler);
		// Host -> Connection
		// std::map<std::string, std::list<client_connection>> connections_;
		int conn_per_host;
		tcp::resolver resolver_;
		ssl::context  context_;
	};
}
}

#include "client.ipp"
