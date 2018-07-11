#pragma once

namespace flame {
namespace udp {
	extern std::unique_ptr<boost::asio::ip::udp::resolver> resolver_;
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}