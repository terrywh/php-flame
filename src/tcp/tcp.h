#pragma once

namespace flame {
namespace tcp {
	extern std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}